#include "ws2307.h"

// fuehrender * bedeutet: Beschriftung und kein Button
#define BUTANZ  6
char *control_buttons_english[BUTANZ] =
{
  (char *)"*ON / off controls",
  (char *)"OFF",
  (char *)"ON",
  (char *)"STANDBY",
  (char *)"ACTIVE",
  (char *)"*Return to main page",
};

char *control_buttons_ger[BUTANZ] =
{
  (char *)"*EIN / AUS Steuerung",
  (char *)"NOTAUS",
  (char *)"EIN",
  (char *)"STANDBY",
  (char *)"AKTIV",
  (char *)"*zur Startseite",
};



int handle_control()
{
  // read data from url
  if (server.hasArg("secret"))
  {
    String s_secret = server.arg("secret");
    String s_accesscode = accesscode;

    if (s_secret == s_accesscode)
    {
      for(int i=0; i<BUTANZ; i++)
      {
        if(server.hasArg(sprache?control_buttons_english[i]:control_buttons_ger[i]))
        {
          switch (i)
          {
            case 1: switchDSP7_off = 1; break;
            case 2: switchDSP7_on = 1; break;
            case 3: switchDSP7_standby = 1; break;
            case 4: switchDSP7_active = 1; break;
          }
          break;
        }
      }

      makeControlHTML(s_secret);

      return 1;
    }
    else
    {
      server.send(404, "text/plain", "Zugang verweigert / Access denied");
    }
  }
  else
  {
    makeControlHTML("");
  }
  return 0;
}


const char control_tit1[] PROGMEM = R"=====(
<!doctype html><head><meta name="viewport" content="width=device-width, initial-scale=1"><title>DSP-7 CONTROL</title><style>
body{background-color:#cccccc;font-family:Arial,Helvetica,Sans-Serif;Color:#000088;}

.greentitle{
padding:8px;
margin-bottom:8px;
background-color:#3333fa;
font-size:22px;
font-weight:bold;
border-radius:12px;
color:white;
}

.bt{
background-color:#3333fa;
border:none;
color:white;
padding:6px 20px;
text-align:center;
text-decoration:none;
display:inline-block;
font-size:16px;
font-weight:bold;
margin:4px 2px;
cursor:pointer;
border-radius:12px;
width: 200px;
}

p {
font-family: verdana;
font-weight:bold;
font-size: 24px;
}
}
</style>
</head>
<body><p><a class="greentitle"><img width="32" height="25" src="
)=====";


const char control_tit2[] PROGMEM =
"\" alt=\" \"/>\
  DSP-7 CONTROL</a></p>\
<form NAME=\"DSP7CTRL\">\
<b><font color=\"#0000FF\">Passcode:   </font></b><br>\
<input type=\"text\" name=\"secret\" value=\"";


const char control_tit2_ger[] PROGMEM =
"\" alt=\" \"/>\
  DSP-7 CONTROL</a></p>\
<form NAME=\"DSP7CTRL\">\
<b><font color=\"#0000FF\">Zugangskennung:   </font></b><br>\
<input type=\"text\" name=\"secret\" value=\"";


void make_buttons()
{
char text[200+1];

  for(int i=0; i<BUTANZ; i++)
  {
    char *p = sprache?control_buttons_english[i]:control_buttons_ger[i];

    if(p[0] != '*')
      snprintf(text,200,"<button class=\"bt\" type=\"submit\" name=\"%s\">%s</button><br>",p,p);
     else
      snprintf(text,200,"<p>%s</p>",p+1);

    html_send_ram(text);
  }

  html_send_ram((char *)"<a href=\"/\" target=\"_self\" class=\"bt\">RETURN</a><br>");
}


// Inline SVG arc gauge. Background grey arc is drawn first; the value arc
// shares the same path so its stroke-dashoffset can be tweened to "fill"
// the gauge. Half-circle 80px-radius arc length = pi * 80 ~= 251.
static const char gauge_svg[] PROGMEM =
"<div style=\"display:flex;justify-content:center;gap:30px;flex-wrap:wrap;margin:12px 0;\">"
  "<div style=\"text-align:center;\">"
    "<svg viewBox=\"0 0 200 130\" width=\"200\" height=\"130\">"
      "<path d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#ddd\" stroke-width=\"18\" fill=\"none\" stroke-linecap=\"round\"/>"
      "<path id=\"powerArc\" d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#3333fa\" stroke-width=\"18\" fill=\"none\" stroke-linecap=\"round\""
            " stroke-dasharray=\"251\" stroke-dashoffset=\"251\" style=\"transition:stroke-dashoffset 0.1s linear;\"/>"
      "<text id=\"powerText\" x=\"100\" y=\"88\" text-anchor=\"middle\" font-size=\"26\" font-weight=\"bold\" fill=\"#000088\">--</text>"
      "<text x=\"100\" y=\"118\" text-anchor=\"middle\" font-size=\"12\" fill=\"#555\">Watts</text>"
    "</svg>"
    "<div style=\"font-weight:bold;color:#000088;\">Power (K-1)</div>"
  "</div>"
  "<div style=\"text-align:center;\">"
    "<svg viewBox=\"0 0 200 130\" width=\"200\" height=\"130\">"
      "<path d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#ddd\" stroke-width=\"18\" fill=\"none\" stroke-linecap=\"round\"/>"
      "<path id=\"swrArc\" d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#00aa00\" stroke-width=\"18\" fill=\"none\" stroke-linecap=\"round\""
            " stroke-dasharray=\"251\" stroke-dashoffset=\"251\" style=\"transition:stroke-dashoffset 0.1s linear,stroke 0.2s;\"/>"
      "<text id=\"swrText\" x=\"100\" y=\"88\" text-anchor=\"middle\" font-size=\"26\" font-weight=\"bold\" fill=\"#000088\">--</text>"
      "<text x=\"100\" y=\"118\" text-anchor=\"middle\" font-size=\"12\" fill=\"#555\">:1</text>"
    "</svg>"
    "<div style=\"font-weight:bold;color:#000088;\">SWR (K-1)</div>"
  "</div>"
"</div>";

static const char tci_panel[] PROGMEM =
"<div style=\"margin:8px auto;padding:10px;background:#f4f4f4;border:1px solid #888;border-radius:8px;font-family:monospace;max-width:380px;\">"
  "<b>Thetis TCI</b> &nbsp; Status: <span id=\"tci_status\">--</span> &nbsp; PTT: <span id=\"tci_ptt\">--</span><br>"
  "RX1: <span id=\"tci_vfo_a\">--</span> MHz <span id=\"tci_a_active\"></span><br>"
  "RX2: <span id=\"tci_vfo_b\">--</span> MHz <span id=\"tci_b_active\"></span>"
"</div>";

void makeControlHTML(String s_secret)
{
char text[200+1];

  html_StartPage();

  // Title bar (head + opening body + branded title)
  html_send_progmem(control_tit1);
  html_send_progmem(wximage);
  html_send_ram((char *)"\" alt=\" \"/>  DSP-7 CONTROL</a></p>");

  // 1) TCI status (top of page)
  html_send_progmem(tci_panel);

  // 2) Coupler-1 gauges
  html_send_progmem(gauge_svg);

  // 3) Passcode + control buttons
  html_send_ram((char *)
    "<form NAME=\"DSP7CTRL\">"
    "<b><font color=\"#0000FF\">");
  html_send_ram((char *)(sprache ? "Passcode:" : "Zugangskennung:"));
  html_send_ram((char *)"</font></b><br>"
    "<input type=\"text\" name=\"secret\" value=\"");
  snprintf(text, sizeof(text), "%s\"><br><br>", const_cast<char*>(s_secret.c_str()));
  html_send_ram(text);

  make_buttons();
  html_send_ram((char *)"</form>");

  // WebSocket handler keeps the TCI panel and gauges live
  buildJavascript_control();
  html_send_ram((char *)"</body></html>");

  html_EndPage();
}
