#include "ws2307.h"

// Page-side JavaScript helpers. Browsers open a WebSocket to port 81
// (served by wsserver.cpp); each push is a JSON state snapshot. Each page
// emits its own onmessage handler that picks the fields it needs.

// kept as a no-op so any old `<body onload='process()'>` markup doesn't
// throw a ReferenceError after the rewrite.
static const char WS_ScriptBegin[] PROGMEM = R"=====(
<SCRIPT>
function process(){}
function connectWS(){
 var ws=new WebSocket('ws://'+location.hostname+':81/');
 ws.onmessage=function(e){
  var d=JSON.parse(e.data);
)=====";

static const char WS_ScriptEnd[] PROGMEM = R"=====(
 };
 ws.onclose=function(){setTimeout(connectWS,1000);};
}
connectWS();
</SCRIPT>
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
  char text[1500];
  html_send_progmem(WS_ScriptBegin);

  // Header status strip + TCI panel
  html_send_ram((char *)
    "if(d.time)document.getElementById('hdr_time').innerHTML=d.time;"
    "if(d.ip)document.getElementById('hdr_ip').innerHTML=d.ip;"
    "if(d.wifi)document.getElementById('hdr_wifi').innerHTML=d.wifi;"
    "if(d.rssi)document.getElementById('hdr_rssi').innerHTML=d.rssi;"
    "if(d.tci_status!==undefined)document.getElementById('tci_status').innerHTML=d.tci_status;"
    "if(d.tci_vfo_a!==undefined)document.getElementById('tci_vfo_a').innerHTML=d.tci_vfo_a;"
    "if(d.tci_vfo_b!==undefined)document.getElementById('tci_vfo_b').innerHTML=d.tci_vfo_b;"
    "if(d.tci_a_active!==undefined)document.getElementById('tci_a_active').innerHTML=d.tci_a_active;"
    "if(d.tci_b_active!==undefined)document.getElementById('tci_b_active').innerHTML=d.tci_b_active;"
    "if(d.tci_ptt!==undefined)document.getElementById('tci_ptt').innerHTML=d.tci_ptt;"
  );

  // Coupler power/SWR helper, plus generic gauge helper for the supply/temp
  // gauges. d.rows[] indexes match the enum order in ws2307.h.
  snprintf(text, sizeof(text),
    "if(d.rows){"
      "function uG(i,pi,si){"
        "var p=parseFloat(d.rows[pi])||0;"
        "document.getElementById('powerText'+i).innerHTML=p.toFixed(1);"
        "document.getElementById('powerArc'+i).style.strokeDashoffset=251*(1-Math.min(1,p/1500));"
        "var s=parseFloat(d.rows[si])||1;"
        "document.getElementById('swrText'+i).innerHTML=s.toFixed(2);"
        "document.getElementById('swrArc'+i).style.strokeDashoffset=251*(1-Math.min(1,Math.max(0,(s-1)/4)));"
        "document.getElementById('swrArc'+i).style.stroke=s>2.5?'#cc0000':(s>1.5?'#cc8800':'#00aa00');"
      "}"
      "uG(1,%d,%d);uG(2,%d,%d);uG(3,%d,%d);"
      "function vG(id,idx,mx,dec){"
        "var v=parseFloat(d.rows[idx])||0;"
        "document.getElementById('txt_'+id).innerHTML=v.toFixed(dec);"
        "document.getElementById('arc_'+id).style.strokeDashoffset=251*(1-Math.min(1,Math.max(0,v/mx)));"
      "}"
      "vG('dcv',%d,60,1);vG('dci',%d,40,1);vG('dcp',%d,2400,0);vG('eff',%d,100,0);"
      "vG('t1',%d,80,1);vG('t2',%d,80,1);vG('fan',%d,100,0);"
      // Temp color: green <50, orange <65, red >=65
      "var t1=parseFloat(d.rows[%d])||0;document.getElementById('arc_t1').style.stroke=t1>=65?'#cc0000':(t1>=50?'#cc8800':'#00aa00');"
      "var t2=parseFloat(d.rows[%d])||0;document.getElementById('arc_t2').style.stroke=t2>=65?'#cc0000':(t2>=50?'#cc8800':'#00aa00');"
      // Band/antenna badges
      "document.getElementById('hdr_band').innerHTML=d.rows[%d]||'--';"
      "document.getElementById('hdr_ant').innerHTML=d.rows[%d]||'--';"
      // Op state badge + PTT highlight
      "var st=d.rows[%d]||'--';document.getElementById('hdr_state').innerHTML=st;"
      "var pt=d.rows[%d]||'--';var pe=document.getElementById('hdr_ptt');"
      "pe.innerHTML=pt;pe.style.background=(/TX|ON|tx|on/.test(pt))?'#cc0000':'#444';"
    "}",
    B1_PWR, B1_SWR, B2_PWR, B2_SWR, B3_PWR, B3_SWR,
    DCVOLT, DCAMP, DCPWR, EFFICIENCY,
    TEMPERATURE1, TEMPERATURE2, FAN,
    TEMPERATURE1, TEMPERATURE2,
    BAND_SELECTED, ANTENNA_SELECTED,
    OPSTATE, PTT);
  html_send_ram(text);

  html_send_progmem(WS_ScriptEnd);
}
