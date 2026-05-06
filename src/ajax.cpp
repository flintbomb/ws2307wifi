#include "ws2307.h"

// Page-side JavaScript helpers. Browsers open a WebSocket to port 81
// (served by wsserver.cpp); each push is a JSON state snapshot. Each page
// emits its own onmessage handler that picks the fields it needs.

// kept as a no-op so any old `<body onload='process()'>` markup doesn't
// throw a ReferenceError after the rewrite.
//
// onmessage body is wrapped in try/catch so a runtime error in any one
// section can't take out subsequent field updates — symptoms used to be
// "the entire page stops updating" if any single line threw.
static const char WS_ScriptBegin[] PROGMEM = R"=====(
<SCRIPT>
function process(){}
function connectWS(){
 var ws=new WebSocket('ws://'+location.hostname+':81/');
 ws.onmessage=function(e){
  var d;try{d=JSON.parse(e.data);}catch(err){return;}
  try{
)=====";

static const char WS_ScriptEnd[] PROGMEM = R"=====(
  }catch(err){if(window.console)console.error('ws handler:',err);}
 };
 ws.onclose=function(){setTimeout(connectWS,1000);};
}
connectWS();
</SCRIPT>
)=====";

// Wraps trend_js in its own <SCRIPT> block so the functions live at top
// level (defined once on page load) instead of being re-declared inside
// the onmessage handler every tick.
static const char trend_script_open[]  PROGMEM = "<SCRIPT>";
static const char trend_script_close[] PROGMEM = "</SCRIPT>";

// Trend graph helpers. Plotted on a 760x260 <canvas id="trendCanvas">
// emitted by control.cpp. Two series share the X axis (last 5 min):
// blue = antenna power (left axis, 0..pmx), orange = temperature 1
// (right axis, tmin..tmx). Rendering is throttled to ~4 Hz to keep the
// load light on slow browsers while WS pushes hit at ~10 Hz.
static const char trend_js[] PROGMEM = R"=====(
window._trend=window._trend||{samples:[],lastDraw:0};
function trendPush(p,t){
  var now=Date.now(),s=window._trend.samples;
  s.push([now,p,t]);
  var co=now-300000;
  while(s.length&&s[0][0]<co)s.shift();
}
function trendDraw(pmx,tmx,tmin,tu){
  var now=Date.now();
  if(now-window._trend.lastDraw<250)return;
  window._trend.lastDraw=now;
  var c=document.getElementById('trendCanvas');
  if(!c||!c.getContext)return;
  var par=c.parentElement;
  while(par&&par.tagName!=='DETAILS')par=par.parentElement;
  if(par&&!par.open)return;
  var ctx=c.getContext('2d'),W=c.width,H=c.height;
  var PL=50,PR=50,PT=20,PB=30,GW=W-PL-PR,GH=H-PT-PB;
  ctx.clearRect(0,0,W,H);
  ctx.strokeStyle='#eee';ctx.lineWidth=1;ctx.beginPath();
  for(var i=0;i<=4;i++){var y=PT+GH*i/4;ctx.moveTo(PL,y);ctx.lineTo(W-PR,y);}
  for(var j=0;j<=5;j++){var x=PL+GW*j/5;ctx.moveTo(x,PT);ctx.lineTo(x,H-PB);}
  ctx.stroke();
  ctx.strokeStyle='#888';ctx.beginPath();
  ctx.moveTo(PL,PT);ctx.lineTo(PL,H-PB);ctx.lineTo(W-PR,H-PB);ctx.lineTo(W-PR,PT);ctx.lineTo(PL,PT);
  ctx.stroke();
  ctx.font='11px sans-serif';
  ctx.fillStyle='#3333fa';ctx.textAlign='right';
  for(var i=0;i<=4;i++){var v=pmx*(1-i/4);ctx.fillText(v.toFixed(0)+'W',PL-4,PT+GH*i/4+4);}
  ctx.fillStyle='#cc4400';ctx.textAlign='left';
  var tustr=(tu||'').replace(/&deg;/g,'°');
  for(var i=0;i<=4;i++){var v=tmin+(tmx-tmin)*(1-i/4);ctx.fillText(v.toFixed(0)+tustr,W-PR+4,PT+GH*i/4+4);}
  ctx.fillStyle='#555';ctx.textAlign='center';
  for(var j=0;j<=5;j++){var x=PL+GW*j/5;var m=-5+j;ctx.fillText((m===0?'now':m+'m'),x,H-PB+14);}
  var s=window._trend.samples;
  if(s.length<2)return;
  var T0=now-300000,span=now-T0;
  function xOf(t){return PL+GW*(t-T0)/span;}
  function yP(p){return PT+GH*(1-Math.max(0,Math.min(1,p/pmx)));}
  function yT(tt){var d=tmx-tmin;if(d<=0)return PT+GH;return PT+GH*(1-Math.max(0,Math.min(1,(tt-tmin)/d)));}
  ctx.lineWidth=2;
  ctx.strokeStyle='#3333fa';ctx.beginPath();var started=false;
  for(var k=0;k<s.length;k++){var pt=s[k];if(pt[0]<T0)continue;var x=xOf(pt[0]),y=yP(pt[1]);if(!started){ctx.moveTo(x,y);started=true;}else ctx.lineTo(x,y);}
  ctx.stroke();
  ctx.strokeStyle='#cc4400';ctx.beginPath();started=false;
  for(var k=0;k<s.length;k++){var pt=s[k];if(pt[0]<T0)continue;var x=xOf(pt[0]),y=yT(pt[2]);if(!started){ctx.moveTo(x,y);started=true;}else ctx.lineTo(x,y);}
  ctx.stroke();
}
)=====";

void buildJavascript()
{
  html_send_progmem(WS_ScriptBegin);
  html_send_ram((char *)
    "var t=document.getElementById('largetab');"
    "if(d.time)t.rows[0].cells[1].innerHTML=d.time;"
    "if(d.ip)t.rows[19].cells[1].innerHTML=d.ip;"
    "if(d.wifi)t.rows[20].cells[1].innerHTML=d.wifi;"
    "if(d.rssi)t.rows[21].cells[1].innerHTML=d.rssi;"
    "if(d.rows)for(var i=0;i<d.rows.length;i++)t.rows[i+1].cells[1].innerHTML=d.rows[i];"
  );
  html_send_progmem(WS_ScriptEnd);
}

void buildJavascript_control()
{
  // Static so the scratch buffer doesn't sit on the cont task stack —
  // ESP8266 only has ~4 KB to share with the framework. Buffer must
  // hold the entire formatted onmessage body; sized with headroom so
  // future tweaks don't silently truncate JS mid-statement (which
  // surfaces in the browser as "missing ) after argument list").
  static char text[4096];

  // Emit the trend helpers in their own <script> first so they're
  // defined at top-level page scope. Also stash whether the passcode is
  // required so the click-handler can skip its empty-passcode warning
  // when the user has turned the gate off in setup.
  html_send_progmem(trend_script_open);
  html_send_progmem(trend_js);
  char pc[60];
  snprintf(pc, sizeof(pc), "window._pcReq=%u;", (unsigned)require_passcode);
  html_send_ram(pc);
  html_send_progmem(trend_script_close);

  html_send_progmem(WS_ScriptBegin);

  // Intercept the control-panel form so OFF / ON / STANDBY / ACTIVE don't
  // reload the page. Fire the request as a fetch and let the websocket
  // push the new state. Only attach once per WS message — guard with a
  // window flag.
  html_send_ram((char *)
    "if(!window._ctrl_hooked){"
      "window._ctrl_hooked=true;"
      "var f=document.forms&&document.forms['DSP7CTRL'];"
      "if(f){"
        "f.querySelectorAll('button').forEach(function(b){"
          "b.addEventListener('click',function(e){"
            "e.preventDefault();"
            "var sec=(f.elements['secret']&&f.elements['secret'].value)||'';"
            "if(window._pcReq&&!sec){"
              "var s=f.elements['secret'];"
              "if(s){s.focus();s.style.outline='2px solid #ff0000';"
                "setTimeout(function(){s.style.outline='';},1500);}"
              "alert('Enter passcode in the header before sending control commands.');"
              "return;"
            "}"
            "var url='/control.php?secret='+encodeURIComponent(sec)+"
                    "'&'+encodeURIComponent(b.name)+'=1';"
            "fetch(url,{method:'GET',cache:'no-store'}).then(function(r){"
              "if(!r.ok){"
                "if(r.status===404)alert('Wrong passcode (server returned 404 / access denied).');"
                "else alert('Control request failed: HTTP '+r.status);"
              "}"
            "}).catch(function(){alert('Control request failed: network error');});"
          "});"
        "});"
      "}"
    "}"
  );

  // Header status strip + TCI panel
  html_send_ram((char *)
    "if(d.time)document.getElementById('hdr_time').innerHTML=d.time;"
    "if(d.ip)document.getElementById('hdr_ip').innerHTML=d.ip;"
    "if(d.wifi)document.getElementById('hdr_wifi').innerHTML=d.wifi;"
    "if(d.rssi)document.getElementById('hdr_rssi').innerHTML=d.rssi;"
    "if(d.tci_status!==undefined)document.getElementById('tci_status').innerHTML=d.tci_status;"
    "if(d.tci_vfo_a!==undefined)document.getElementById('tci_vfo_a').innerHTML=d.tci_vfo_a;"
    "if(d.tci_vfo_b!==undefined)document.getElementById('tci_vfo_b').innerHTML=d.tci_vfo_b;"
    "if(d.tci_a_active!==undefined){"
      "document.getElementById('tci_a_active').innerHTML=d.tci_a_active;"
      "document.getElementById('tci_row_a').classList.toggle('tci-active',d.tci_a_active!=='');"
    "}"
    "if(d.tci_b_active!==undefined){"
      "document.getElementById('tci_b_active').innerHTML=d.tci_b_active;"
      "document.getElementById('tci_row_b').classList.toggle('tci-active',d.tci_b_active!=='');"
    "}"
    "if(d.tci_ptt!==undefined)document.getElementById('tci_ptt').innerHTML=d.tci_ptt;"
  );

  // Coupler power/SWR helper, plus generic gauge helper for the supply/temp
  // gauges. d.rows[] indexes match the enum order in ws2307.h.
  snprintf(text, sizeof(text),
    "if(d.rows){"
      // 2-second peak-hold for power/SWR text. Per-key sliding window keyed
      // by 'p1','s1','p2','s2',... Updated every WS push (~10 Hz).
      "window._mh=window._mh||{};"
      "function mh(k,v){var t=Date.now(),a=window._mh[k]=window._mh[k]||[];"
        "a.push([t,v]);while(a.length&&a[0][0]<t-2000)a.shift();"
        "var m=a[0][1];for(var i=1;i<a.length;i++)if(a[i][1]>m)m=a[i][1];return m;}"
      // pmx = K-1/K-2 max from Drive_limit +5%; K-3 stays on a fixed 1500W
      // sweep since it's the input coupler and seldom exceeds that.
      "var pmx=parseFloat(d.pwr_max)||1050;"
      "function uG(i,pi,si,mxw){"
        "var p=parseFloat(d.rows[pi])||0;"
        "var ph=mh('p'+i,p);"
        "document.getElementById('powerText'+i).innerHTML=ph.toFixed(1);"
        "document.getElementById('powerArc'+i).style.strokeDashoffset=251*(1-Math.min(1,p/mxw));"
        "var s=parseFloat(d.rows[si])||1;"
        "var sh=mh('s'+i,s);"
        "document.getElementById('swrText'+i).innerHTML=sh.toFixed(2);"
        "document.getElementById('swrArc'+i).style.strokeDashoffset=251*(1-Math.min(1,Math.max(0,(s-1)/4)));"
        "document.getElementById('swrArc'+i).style.stroke=s>2.5?'#cc0000':(s>1.5?'#cc8800':'#00aa00');"
      "}"
      "uG(1,%d,%d,pmx);uG(2,%d,%d,pmx);uG(3,%d,%d,1500);"
      "function vG(id,idx,mx,dec){"
        "var v=parseFloat(d.rows[idx])||0;"
        "document.getElementById('txt_'+id).innerHTML=v.toFixed(dec);"
        "document.getElementById('arc_'+id).style.strokeDashoffset=251*(1-Math.min(1,Math.max(0,v/mx)));"
      "}"
      "vG('dcv',%d,60,1);vG('dci',%d,40,1);vG('dcp',%d,2400,0);vG('eff',%d,100,0);"
      // Temperature gauge range = Temp_limit + 10 (from DSP-7 config dump,
      // pushed via d.temp_limit). Unit label tracks d.temp_unit (also
      // sourced from the config dump tempunits flag).
      "var tl=parseFloat(d.temp_limit)||80;"
      "var tu=d.temp_unit||'&deg;';"
      "vG('t1',%d,tl,1);vG('t2',%d,tl,1);vG('fan',%d,100,0);"
      "var u1=document.getElementById('unit_t1');if(u1)u1.innerHTML=tu;"
      "var u2=document.getElementById('unit_t2');if(u2)u2.innerHTML=tu;"
      // Temp color thresholds rebased to Temp_limit: red at/above limit,
      // orange in the upper ~25%% (limit-15), green below.
      "var lim=parseFloat(d.temp_limit)?parseFloat(d.temp_limit)-10:65;"
      "var t1=parseFloat(d.rows[%d])||0;document.getElementById('arc_t1').style.stroke=t1>=lim?'#cc0000':(t1>=lim-15?'#cc8800':'#00aa00');"
      "var t2=parseFloat(d.rows[%d])||0;document.getElementById('arc_t2').style.stroke=t2>=lim?'#cc0000':(t2>=lim-15?'#cc8800':'#00aa00');"
      // Band/antenna badges
      "document.getElementById('hdr_band').innerHTML=d.rows[%d]||'--';"
      "document.getElementById('hdr_ant').innerHTML=d.rows[%d]||'--';"
      // TCI warning banner: enabled but no freq, or freq outside DSP-7 band.
      "var tw=document.getElementById('tci_warn');"
      "if(tw){"
        "var msg='';"
        "if(d.tci_enabled){"
          "var tx=d.tci_tx_freq_hz||0;"
          "var exp=d.tci_band_expected||'';"
          "var cur=(d.rows[%d]||'').toString();"
          "if(tx===0||tx==='0')msg='TCI enabled but no TX frequency received from Thetis';"
          "else if(exp!==''&&cur!==''&&cur.indexOf(exp)===-1)"
            "msg='TX freq band ('+exp+'m) does not match DSP-7 band ('+cur+')';"
        "}"
        "tw.style.display=msg?'flex':'none';"
        "tw.innerHTML=msg;"
      "}"
      // Op state badge + PTT highlight + STANDBY/ACTIVE toggle pill
      "var st=d.rows[%d]||'--';document.getElementById('hdr_state').innerHTML=st;"
      "var su=(st||'').toString().trim().toUpperCase();"
      // DSP-7 emits 'OPERATION' for active, 'STANDBY' for standby,
      // 'EMERG.OFF' for off, 'POWERUP' transitional (see ws2307_evaluate.cpp).
      // ON-half is current whenever the radio is powered (STANDBY or OPERATION).
      "var isAct=(su==='OPERATION');"
      "var isStb=(su==='STANDBY');"
      "var isOff=(su==='EMERG.OFF');"
      "var isOn=(isAct||isStb);"
      "var bs=document.getElementById('btn_standby');var ba=document.getElementById('btn_active');"
      "var boff=document.getElementById('btn_off');var bon=document.getElementById('btn_on');"
      "if(bs&&ba){"
        "bs.classList.toggle('is-current',isStb);"
        "ba.classList.toggle('is-current',isAct);"
      "}"
      "if(boff&&bon){"
        "boff.classList.toggle('is-current',isOff);"
        "bon.classList.toggle('is-current',isOn);"
      "}"
      "var pt=d.rows[%d]||'--';var pe=document.getElementById('hdr_ptt');"
      "pe.innerHTML=pt;pe.style.background=(/TX|ON|tx|on/.test(pt))?'#cc0000':'#444';"
      // Trend graph: push antenna power + temperature 1 each tick, then
      // redraw (the draw helper throttles itself to ~4 Hz).
      "trendPush(parseFloat(d.rows[%d])||0,parseFloat(d.rows[%d])||0);"
      "trendDraw(pmx,tl,0,tu);"
    "}",
    B1_PWR, B1_SWR, B2_PWR, B2_SWR, B3_PWR, B3_SWR,
    DCVOLT, DCAMP, DCPWR, EFFICIENCY,
    TEMPERATURE1, TEMPERATURE2, FAN,
    TEMPERATURE1, TEMPERATURE2,
    BAND_SELECTED, ANTENNA_SELECTED,
    BAND_SELECTED,    // for the TCI warning banner band-match check
    OPSTATE, PTT,
    B1_PWR, TEMPERATURE1);
  html_send_ram(text);

  html_send_progmem(WS_ScriptEnd);
}
