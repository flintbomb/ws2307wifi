#include "ws2307.h"

// fuehrender * bedeutet: Beschriftung und kein Button
#define BUTANZ  5
char *control_buttons_english[BUTANZ] =
{
  (char *)"*ON / off controls",
  (char *)"OFF",
  (char *)"ON",
  (char *)"STANDBY",
  (char *)"ACTIVE",
};

char *control_buttons_ger[BUTANZ] =
{
  (char *)"*EIN / AUS Steuerung",
  (char *)"NOTAUS",
  (char *)"EIN",
  (char *)"STANDBY",
  (char *)"AKTIV",
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
}


// Emit a single coupler row: power gauge + SWR gauge side by side, with
// IDs powerArcN/powerTextN/swrArcN/swrTextN so the JS handler can update
// each independently. Half-circle 80px-radius arc length = pi*80 ~= 251.
static void emit_coupler_row(int n, const char *labelP, const char *labelS)
{
  char buf[2048];
  snprintf(buf, sizeof(buf),
    "<div style=\"display:flex;justify-content:center;gap:30px;flex-wrap:wrap;margin:8px 0;\">"
      "<div style=\"text-align:center;\">"
        "<svg viewBox=\"0 0 200 130\" width=\"180\" height=\"117\">"
          "<path d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#ddd\" stroke-width=\"18\" fill=\"none\" stroke-linecap=\"round\"/>"
          "<path id=\"powerArc%d\" d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#3333fa\" stroke-width=\"18\" fill=\"none\" stroke-linecap=\"round\""
                " stroke-dasharray=\"251\" stroke-dashoffset=\"251\" style=\"transition:stroke-dashoffset 0.1s linear;\"/>"
          "<text id=\"powerText%d\" x=\"100\" y=\"88\" text-anchor=\"middle\" font-size=\"26\" font-weight=\"bold\" fill=\"#000088\">--</text>"
          "<text x=\"100\" y=\"118\" text-anchor=\"middle\" font-size=\"12\" fill=\"#555\">Watts</text>"
        "</svg>"
        "<div style=\"font-weight:bold;color:#000088;\">%s</div>"
      "</div>"
      "<div style=\"text-align:center;\">"
        "<svg viewBox=\"0 0 200 130\" width=\"180\" height=\"117\">"
          "<path d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#ddd\" stroke-width=\"18\" fill=\"none\" stroke-linecap=\"round\"/>"
          "<path id=\"swrArc%d\" d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#00aa00\" stroke-width=\"18\" fill=\"none\" stroke-linecap=\"round\""
                " stroke-dasharray=\"251\" stroke-dashoffset=\"251\" style=\"transition:stroke-dashoffset 0.1s linear,stroke 0.2s;\"/>"
          "<text id=\"swrText%d\" x=\"100\" y=\"88\" text-anchor=\"middle\" font-size=\"26\" font-weight=\"bold\" fill=\"#000088\">--</text>"
          "<text x=\"100\" y=\"118\" text-anchor=\"middle\" font-size=\"12\" fill=\"#555\">:1</text>"
        "</svg>"
        "<div style=\"font-weight:bold;color:#000088;\">%s</div>"
      "</div>"
    "</div>",
    n, n, labelP,
    n, n, labelS);
  html_send_ram(buf);
}

static const char tci_panel[] PROGMEM =
"<div style=\"margin:8px auto;padding:10px;background:#f4f4f4;border:1px solid #888;border-radius:8px;font-family:monospace;max-width:380px;\">"
  "<b>Thetis TCI</b> &nbsp; Status: <span id=\"tci_status\">--</span> &nbsp; PTT: <span id=\"tci_ptt\">--</span><br>"
  "RX1: <span id=\"tci_vfo_a\">--</span> MHz <span id=\"tci_a_active\"></span><br>"
  "RX2: <span id=\"tci_vfo_b\">--</span> MHz <span id=\"tci_b_active\"></span>"
"</div>";

// Top status bar: time, IP, WiFi, RSSI, op state, PTT
static const char status_bar[] PROGMEM =
"<div style=\"margin:8px auto;padding:8px 14px;background:#222;color:#eee;border-radius:8px;"
"max-width:780px;display:flex;flex-wrap:wrap;gap:10px 22px;justify-content:center;font-family:monospace;font-size:13px;\">"
  "<span><b>Time</b> <span id=\"hdr_time\">--</span></span>"
  "<span><b>IP</b> <span id=\"hdr_ip\">--</span></span>"
  "<span><b>WiFi</b> <span id=\"hdr_wifi\">--</span></span>"
  "<span><b>RSSI</b> <span id=\"hdr_rssi\">--</span></span>"
  "<span><b>State</b> <span id=\"hdr_state\">--</span></span>"
  "<span><b>PTT</b> <span id=\"hdr_ptt\" style=\"padding:2px 8px;border-radius:4px;background:#444;\">--</span></span>"
"</div>";

// Generic small arc gauge (one column). idSuffix lets multiple coexist.
// Renders an arc that fills proportional to value/max, plus a numeric
// readout with a unit suffix below the dial.
static void emit_value_gauge(const char *idSuffix, const char *units,
                             const char *label, const char *colorHi)
{
  char buf[1500];
  snprintf(buf, sizeof(buf),
    "<div style=\"text-align:center;\">"
      "<svg viewBox=\"0 0 200 130\" width=\"150\" height=\"98\">"
        "<path d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#ddd\" stroke-width=\"16\" fill=\"none\" stroke-linecap=\"round\"/>"
        "<path id=\"arc_%s\" d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"%s\" stroke-width=\"16\" fill=\"none\" stroke-linecap=\"round\""
              " stroke-dasharray=\"251\" stroke-dashoffset=\"251\" style=\"transition:stroke-dashoffset 0.1s linear,stroke 0.2s;\"/>"
        "<text id=\"txt_%s\" x=\"100\" y=\"88\" text-anchor=\"middle\" font-size=\"24\" font-weight=\"bold\" fill=\"#000088\">--</text>"
        "<text x=\"100\" y=\"118\" text-anchor=\"middle\" font-size=\"11\" fill=\"#555\">%s</text>"
      "</svg>"
      "<div style=\"font-weight:bold;color:#000088;font-size:13px;\">%s</div>"
    "</div>",
    idSuffix, colorHi, idSuffix, units, label);
  html_send_ram(buf);
}

// Section heading (small caps blue band)
static void emit_section(const char *title)
{
  char buf[200];
  snprintf(buf, sizeof(buf),
    "<div style=\"margin:14px auto 4px;padding:4px 12px;background:#3333fa;color:white;"
    "max-width:780px;border-radius:6px;font-weight:bold;font-size:14px;letter-spacing:1px;\">%s</div>",
    title);
  html_send_ram(buf);
}

// Open / close a row container that wraps gauges nicely on small screens.
static void emit_row_open()
{
  html_send_ram((char *)
    "<div style=\"display:flex;justify-content:center;gap:10px;flex-wrap:wrap;margin:4px auto;max-width:780px;\">");
}
static void emit_row_close()
{
  html_send_ram((char *)"</div>");
}

// Two pill badges side by side (band, antenna). Updated by JS via the
// d.rows[] indices.
static const char band_ant_panel[] PROGMEM =
"<div style=\"margin:6px auto;padding:8px;text-align:center;max-width:780px;\">"
  "<span style=\"display:inline-block;padding:6px 18px;margin:4px;background:#444;color:#0fb;"
       "border-radius:8px;font-family:monospace;font-size:16px;font-weight:bold;\">"
    "Band: <span id=\"hdr_band\">--</span>"
  "</span>"
  "<span style=\"display:inline-block;padding:6px 18px;margin:4px;background:#444;color:#fc8;"
       "border-radius:8px;font-family:monospace;font-size:16px;font-weight:bold;\">"
    "Antenna: <span id=\"hdr_ant\">--</span>"
  "</span>"
"</div>";

void makeControlHTML(String s_secret)
{
char text[300+1];

  html_StartPage();

  // Title bar (head + opening body + branded title)
  html_send_progmem(control_tit1);
  html_send_progmem(wximage);
  html_send_ram((char *)"\" alt=\" \"/>  DSP-7 CONTROL</a></p>");

  // The whole page is one form so the passcode in the header is submitted
  // along with whichever ON/OFF/STANDBY/ACTIVE button the user clicks in
  // the side column further down.
  html_send_ram((char *)"<form NAME=\"DSP7CTRL\">");

  // 1) Top status strip — Time / IP / WiFi / RSSI / State / PTT + passcode
  html_send_progmem(status_bar);
  html_send_ram((char *)
    "<div style=\"margin:6px auto;padding:6px 14px;background:#222;color:#eee;border-radius:8px;"
    "max-width:780px;display:flex;flex-wrap:wrap;gap:10px;justify-content:center;align-items:center;"
    "font-family:monospace;font-size:13px;\">"
    "<b style=\"color:#0fb;\">");
  html_send_ram((char *)(sprache ? "Passcode:" : "Zugangskennung:"));
  html_send_ram((char *)"</b>"
    "<input type=\"text\" name=\"secret\" value=\"");
  if (s_secret.length() > 0)
    html_send_ram(const_cast<char*>(s_secret.c_str()));
  html_send_ram((char *)
    "\" size=\"12\" "
    "style=\"padding:3px 8px;border-radius:4px;border:1px solid #555;background:#111;color:#eee;\">"
    "</div>");

  // 2) Thetis TCI panel
  html_send_progmem(tci_panel);

  // 3) Band / Antenna badges (sit directly above the meters + controls)
  html_send_progmem(band_ant_panel);

  // 4) Power & SWR — controls column on the left, three couplers on the right
  emit_section("POWER &amp; SWR");
  html_send_ram((char *)
    "<div style=\"display:flex;justify-content:center;align-items:flex-start;gap:20px;"
    "flex-wrap:wrap;max-width:900px;margin:0 auto;\">"
      "<div style=\"flex:0 0 auto;text-align:center;min-width:220px;\">");
  make_buttons();
  html_send_ram((char *)
      "</div>"
      "<div style=\"flex:1 1 auto;min-width:380px;\">");
  emit_coupler_row(1, "Power (K-1 Antenna)", "SWR (K-1 Antenna)");
  emit_coupler_row(2, "Power (K-2 Filter)",  "SWR (K-2 Filter)");
  emit_coupler_row(3, "Power (K-3 Input)",   "SWR (K-3 Input)");
  html_send_ram((char *)
      "</div>"
    "</div>");

  // 4) Temperature & fan gauges
  emit_section("TEMPERATURE &amp; FAN");
  emit_row_open();
  emit_value_gauge("t1",  "&deg;",       "Temperature 1","#cc4400");
  emit_value_gauge("t2",  "&deg;",       "Temperature 2","#cc4400");
  emit_value_gauge("fan", "%",           "Fan Speed",    "#3333fa");
  emit_row_close();

  // 5) Power supply gauges (DC volts / amps / watts / efficiency)
  emit_section("POWER SUPPLY");
  emit_row_open();
  emit_value_gauge("dcv", "Volts",       "DC Voltage",   "#3333fa");
  emit_value_gauge("dci", "Amps",        "DC Current",   "#3333fa");
  emit_value_gauge("dcp", "Watts",       "DC Power",     "#3333fa");
  emit_value_gauge("eff", "%",           "Efficiency",   "#00aa00");
  emit_row_close();

  // Close the page-spanning form
  html_send_ram((char *)"</form>");

  // 7) Setup / Config links
  html_send_ram((char *)
    "<div style=\"text-align:center;margin:14px;\">"
    "<a href=\"/setup.php\" target=\"_self\" class=\"bt\">SETUP</a> "
    "<a href=\"/config.php\" target=\"_self\" class=\"bt\">CONFIG</a>"
    "</div>");

  // WebSocket handler keeps the entire dashboard live
  buildJavascript_control();
  html_send_ram((char *)"</body></html>");

  html_EndPage();
}
