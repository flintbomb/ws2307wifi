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

void buildJavascript_coupler(char coupnum)
{
  char text[200];
  html_send_progmem(WS_ScriptBegin);
  // B1_PWR/B1_SWR are entries 11/12; coupler N uses (B1_*)+ (N-1)*2
  snprintf(text, sizeof(text),
    "if(d.rows){"
    "document.getElementById('runtime_power').innerHTML=d.rows[%d];"
    "document.getElementById('runtime_swr').innerHTML=d.rows[%d];"
    "}",
    B1_PWR + (coupnum - 1) * 2,
    B1_SWR + (coupnum - 1) * 2);
  html_send_ram(text);
  html_send_progmem(WS_ScriptEnd);
}

void buildJavascript_control()
{
  char text[700];
  html_send_progmem(WS_ScriptBegin);
  html_send_ram((char *)
    "if(d.tci_status!==undefined)document.getElementById('tci_status').innerHTML=d.tci_status;"
    "if(d.tci_vfo_a!==undefined)document.getElementById('tci_vfo_a').innerHTML=d.tci_vfo_a;"
    "if(d.tci_vfo_b!==undefined)document.getElementById('tci_vfo_b').innerHTML=d.tci_vfo_b;"
    "if(d.tci_a_active!==undefined)document.getElementById('tci_a_active').innerHTML=d.tci_a_active;"
    "if(d.tci_b_active!==undefined)document.getElementById('tci_b_active').innerHTML=d.tci_b_active;"
    "if(d.tci_ptt!==undefined)document.getElementById('tci_ptt').innerHTML=d.tci_ptt;"
  );
  // Coupler-1 gauges. Power max = 1500W, SWR scale 1..5, color zones at 1.5/2.5
  snprintf(text, sizeof(text),
    "if(d.rows){"
      "var p=parseFloat(d.rows[%d])||0;"
      "document.getElementById('powerText').innerHTML=p.toFixed(1);"
      "document.getElementById('powerArc').style.strokeDashoffset=251*(1-Math.min(1,p/1500));"
      "var s=parseFloat(d.rows[%d])||1;"
      "document.getElementById('swrText').innerHTML=s.toFixed(2);"
      "document.getElementById('swrArc').style.strokeDashoffset=251*(1-Math.min(1,Math.max(0,(s-1)/4)));"
      "document.getElementById('swrArc').style.stroke=s>2.5?'#cc0000':(s>1.5?'#cc8800':'#00aa00');"
    "}",
    B1_PWR, B1_SWR);
  html_send_ram(text);
  html_send_progmem(WS_ScriptEnd);
}
