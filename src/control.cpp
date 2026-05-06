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

    if (!require_passcode || s_secret == s_accesscode)
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
body{background-color:#cccccc;font-family:Arial,Helvetica,Sans-Serif;Color:#000088;
     max-width:100%;margin:0 auto;padding:0 6px;}

.greentitle{
display:flex;
flex-wrap:wrap;
align-items:center;
justify-content:space-between;
gap:8px 18px;
padding:8px 14px;
margin-bottom:8px;
background-color:#3333fa;
font-size:22px;
font-weight:bold;
border-radius:12px;
color:white;
}
.greentitle .titletext{display:flex;align-items:center;gap:10px;flex:0 0 auto;}
.greentitle .titlestatus{flex:1 1 auto;color:#eef;}
.greentitle .titlestatus b{color:#fff;}
.greentitle .titlestatus span span{color:#9fffd9;}
.greentitle .titlestatus #hdr_ptt{color:#fff;padding:2px 8px;border-radius:4px;background:#222;}

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
<body><div class="greentitle"><span class="titletext"><img width="32" height="25" src="
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


// Indexes inside control_buttons_*[] — keep in sync with the array.
#define BTN_OFF      1
#define BTN_ON       2
#define BTN_STANDBY  3
#define BTN_ACTIVE   4

void make_buttons()
{
  char text[400];
  char **labels = sprache ? control_buttons_english : control_buttons_ger;

  // Section heading (the array entry 0 is the "*ON / off controls" label)
  snprintf(text, sizeof(text), "<p>%s</p>", labels[0] + 1);
  html_send_ram(text);

  // Two pill toggles, identical style. Top: OFF / ON. Bottom: STANDBY /
  // ACTIVE. Each half neutral by default; JS adds .is-current to the one
  // matching the live OPSTATE, which paints OFF red, ON green, STANDBY
  // amber, ACTIVE green.
  html_send_ram((char *)
    "<style>"
    ".bt-pill{display:flex;width:220px;margin:6px 0;border-radius:12px;overflow:hidden;"
      "border:2px solid #444;}"
    ".bt-pill button{flex:1;height:54px;font-size:16px;font-weight:bold;border:none;"
      "cursor:pointer;color:#fff;transition:background 0.2s;background:#777;}"
    ".bt-pill button:hover{background:#999;}"
    ".bt-pill .bt-off.is-current{background:#cc0000 !important;}"
    ".bt-pill .bt-on.is-current{background:#00aa44 !important;}"
    ".bt-pill .bt-standby.is-current{background:#cc8800 !important;}"
    ".bt-pill .bt-active.is-current{background:#00aa44 !important;}"
    "</style>");

  // OFF / ON pill — JS toggles .is-current based on OPSTATE.
  snprintf(text, sizeof(text),
    "<div class=\"bt-pill\">"
      "<button class=\"bt-off\" id=\"btn_off\" type=\"submit\" name=\"%s\">%s</button>"
      "<button class=\"bt-on\"  id=\"btn_on\"  type=\"submit\" name=\"%s\">%s</button>"
    "</div>",
    labels[BTN_OFF], labels[BTN_OFF],
    labels[BTN_ON],  labels[BTN_ON]);
  html_send_ram(text);

  // STANDBY / ACTIVE toggle pill
  snprintf(text, sizeof(text),
    "<div class=\"bt-pill\">"
      "<button class=\"bt-standby\" id=\"btn_standby\" type=\"submit\" name=\"%s\">%s</button>"
      "<button class=\"bt-active\"  id=\"btn_active\"  type=\"submit\" name=\"%s\">%s</button>"
    "</div>",
    labels[BTN_STANDBY], labels[BTN_STANDBY],
    labels[BTN_ACTIVE],  labels[BTN_ACTIVE]);
  html_send_ram(text);
}


// Emit a single coupler row: power gauge + SWR gauge side by side, with
// IDs powerArcN/powerTextN/swrArcN/swrTextN so the JS handler can update
// each independently. Half-circle 80px-radius arc length = pi*80 ~= 251.
//
// The K-1 (antenna) row gets bigger SVGs and a static orange "warning
// band" on the power gauge from 80% -> 100% of the gauge max. The
// dasharray "0 200 50 1" pattern paints only the trailing 20% of the
// 251-unit arc.
//
// Implementation note: we stream chunks via html_send_ram instead of
// buffering everything in a single big char[] on the stack, because the
// ESP8266 has only ~4 KB of stack to share with the framework.
static void emit_coupler_row(int n, const char *labelP, const char *labelS,
                             int big, int warn)
{
  char buf[768];
  int w  = big ? 240 : 180;
  int h  = big ? 156 : 117;
  int sw = big ? 22  : 18;
  int fz = big ? 32  : 26;
  int lz = big ? 14  : 12;
  int rl = big ? 15  : 14;     // row label size

  // Big rows (K-1 Antenna) get a card-style outer wrapper with an accent
  // border, gradient fill, and a corner tag so the antenna meters jump
  // out from the rest of the dashboard.
  if (big) {
    html_send_ram((char *)
      "<div style=\"position:relative;border:3px solid #3333fa;border-radius:14px;"
      "background:linear-gradient(180deg,#ffffff,#eef0ff);"
      "box-shadow:0 4px 14px rgba(51,51,250,0.25);"
      "padding:14px 16px 10px;margin:14px 0 8px;\">"
        "<div style=\"position:absolute;top:-11px;left:18px;background:#3333fa;"
        "color:#fff;padding:2px 12px;border-radius:6px;font-size:11px;"
        "font-weight:bold;letter-spacing:2px;\">ANTENNA</div>");
  }

  html_send_ram((char *)
    "<div style=\"display:flex;justify-content:center;align-items:flex-start;"
                "gap:30px;flex-wrap:wrap;margin:8px 0;\">"
      "<div style=\"text-align:center;\">");

  // Power gauge SVG
  snprintf(buf, sizeof(buf),
    "<svg viewBox=\"0 0 200 130\" width=\"%d\" height=\"%d\">"
    "<path d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#ddd\" "
          "stroke-width=\"%d\" fill=\"none\" stroke-linecap=\"round\"/>",
    w, h, sw);
  html_send_ram(buf);

  if (warn) {
    snprintf(buf, sizeof(buf),
      "<path d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#cc8800\" "
      "stroke-width=\"%d\" fill=\"none\" stroke-linecap=\"round\" "
      "stroke-dasharray=\"0 200 50 1\" opacity=\"0.55\"/>",
      sw);
    html_send_ram(buf);
  }

  snprintf(buf, sizeof(buf),
    "<path id=\"powerArc%d\" d=\"M 20 100 A 80 80 0 0 1 180 100\" "
          "stroke=\"#3333fa\" stroke-width=\"%d\" fill=\"none\" "
          "stroke-linecap=\"round\" stroke-dasharray=\"251\" "
          "stroke-dashoffset=\"251\" style=\"transition:stroke-dashoffset 0.1s linear;\"/>"
    "<text id=\"powerText%d\" x=\"100\" y=\"88\" text-anchor=\"middle\" "
          "font-size=\"%d\" font-weight=\"bold\" fill=\"#000088\">--</text>"
    "<text x=\"100\" y=\"118\" text-anchor=\"middle\" font-size=\"%d\" "
          "fill=\"#555\">Watts (peak 2s)</text>"
    "</svg>",
    n, sw, n, fz, lz);
  html_send_ram(buf);

  snprintf(buf, sizeof(buf),
    "<div style=\"font-weight:bold;color:#000088;font-size:%dpx;\">%s</div>"
    "</div>"
    "<div style=\"text-align:center;\">",
    rl, labelP);
  html_send_ram(buf);

  // SWR gauge SVG
  snprintf(buf, sizeof(buf),
    "<svg viewBox=\"0 0 200 130\" width=\"%d\" height=\"%d\">"
    "<path d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#ddd\" "
          "stroke-width=\"%d\" fill=\"none\" stroke-linecap=\"round\"/>"
    "<path id=\"swrArc%d\" d=\"M 20 100 A 80 80 0 0 1 180 100\" "
          "stroke=\"#00aa00\" stroke-width=\"%d\" fill=\"none\" "
          "stroke-linecap=\"round\" stroke-dasharray=\"251\" "
          "stroke-dashoffset=\"251\" style=\"transition:stroke-dashoffset 0.1s linear,stroke 0.2s;\"/>"
    "<text id=\"swrText%d\" x=\"100\" y=\"88\" text-anchor=\"middle\" "
          "font-size=\"%d\" font-weight=\"bold\" fill=\"#000088\">--</text>"
    "<text x=\"100\" y=\"118\" text-anchor=\"middle\" font-size=\"%d\" "
          "fill=\"#555\">:1 (peak 2s)</text>"
    "</svg>",
    w, h, sw, n, sw, n, fz, lz);
  html_send_ram(buf);

  snprintf(buf, sizeof(buf),
    "<div style=\"font-weight:bold;color:#000088;font-size:%dpx;\">%s</div>"
    "</div></div>",
    rl, labelS);
  html_send_ram(buf);

  // Close the big-row card wrapper opened above.
  if (big) html_send_ram((char *)"</div>");
}

static const char tci_panel[] PROGMEM =
"<style>"
".tcirow{padding:3px 6px;border-radius:6px;font-size:14px;transition:all 0.15s;}"
".tcirow.tci-active{font-size:20px;font-weight:bold;background:#ffe680;color:#000;"
"box-shadow:0 0 0 2px #cc8800;}"
"</style>"
"<div style=\"padding:10px;background:#f4f4f4;border:1px solid #888;border-radius:8px;font-family:monospace;max-width:380px;\">"
  "<b>Thetis TCI</b> &nbsp; Status: <span id=\"tci_status\">--</span> &nbsp; PTT: <span id=\"tci_ptt\">--</span><br>"
  "<div id=\"tci_row_a\" class=\"tcirow\">RX1: <span id=\"tci_vfo_a\">--</span> MHz <span id=\"tci_a_active\"></span></div>"
  "<div id=\"tci_row_b\" class=\"tcirow\">RX2: <span id=\"tci_vfo_b\">--</span> MHz <span id=\"tci_b_active\"></span></div>"
"</div>";

// Top status bar: time, IP, WiFi, RSSI, op state, PTT — embedded inside
// the DSP-7 title bar via the .titlestatus wrapper class so it shares
// the bar's blue background. Bumped one size up from the old 13 px row.
static const char status_bar[] PROGMEM =
"<div class=\"titlestatus\" style=\"display:flex;flex-wrap:wrap;gap:10px 22px;"
"justify-content:center;align-items:center;font-family:monospace;font-size:15px;"
"font-weight:normal;letter-spacing:0;\">"
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
  char buf[900];
  snprintf(buf, sizeof(buf),
    "<div style=\"text-align:center;\">"
      "<svg viewBox=\"0 0 200 130\" width=\"150\" height=\"98\">"
        "<path d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#ddd\" stroke-width=\"16\" fill=\"none\" stroke-linecap=\"round\"/>"
        "<path id=\"arc_%s\" d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"%s\" stroke-width=\"16\" fill=\"none\" stroke-linecap=\"round\""
              " stroke-dasharray=\"251\" stroke-dashoffset=\"251\" style=\"transition:stroke-dashoffset 0.1s linear,stroke 0.2s;\"/>"
        "<text id=\"txt_%s\" x=\"100\" y=\"88\" text-anchor=\"middle\" font-size=\"24\" font-weight=\"bold\" fill=\"#000088\">--</text>"
        "<text id=\"unit_%s\" x=\"100\" y=\"118\" text-anchor=\"middle\" font-size=\"11\" fill=\"#555\">%s</text>"
      "</svg>"
      "<div style=\"font-weight:bold;color:#000088;font-size:13px;\">%s</div>"
    "</div>",
    idSuffix, colorHi, idSuffix, idSuffix, units, label);
  html_send_ram(buf);
}

// Section heading (small caps blue band)
static void emit_section(const char *title)
{
  char buf[200];
  snprintf(buf, sizeof(buf),
    "<div style=\"margin:14px auto 4px;padding:4px 12px;background:#3333fa;color:white;"
    "max-width:100%;border-radius:6px;font-weight:bold;font-size:14px;letter-spacing:1px;\">%s</div>",
    title);
  html_send_ram(buf);
}

// Open / close a row container that wraps gauges nicely on small screens.
static void emit_row_open()
{
  html_send_ram((char *)
    "<div style=\"display:flex;justify-content:center;gap:10px;flex-wrap:wrap;margin:4px auto;max-width:100%;\">");
}
static void emit_row_close()
{
  html_send_ram((char *)"</div>");
}

// Band + Antenna pills, stacked. Sized to sit alongside the TCI panel.
// Updated by JS via the d.rows[] indices (BAND_SELECTED / ANTENNA_SELECTED).
static const char band_ant_panel[] PROGMEM =
"<div style=\"display:flex;flex-direction:column;gap:8px;justify-content:center;\">"
  "<div style=\"padding:10px 22px;background:#444;color:#0fb;"
       "border-radius:10px;font-family:monospace;font-size:22px;font-weight:bold;"
       "text-align:center;min-width:180px;\">"
    "Band: <span id=\"hdr_band\">--</span>"
  "</div>"
  "<div style=\"padding:10px 22px;background:#444;color:#fc8;"
       "border-radius:10px;font-family:monospace;font-size:22px;font-weight:bold;"
       "text-align:center;min-width:180px;\">"
    "Antenna: <span id=\"hdr_ant\">--</span>"
  "</div>"
"</div>";

void makeControlHTML(String s_secret)
{
char text[300+1];

  html_StartPage();

  // Title bar — combined title + status row in a single .greentitle div.
  html_send_progmem(control_tit1);
  html_send_progmem(wximage);
  html_send_ram((char *)"\" alt=\" \"/>  DSP-7 CONTROL</span>");
  html_send_progmem(status_bar);   // emits the .titlestatus row inline
  html_send_ram((char *)"</div>");  // close .greentitle

  // The whole page is one form so the passcode in the header is submitted
  // along with whichever ON/OFF/STANDBY/ACTIVE button the user clicks in
  // the side column further down.
  html_send_ram((char *)"<form NAME=\"DSP7CTRL\">");
  if (require_passcode)
  {
    html_send_ram((char *)
      "<div style=\"margin:6px auto;padding:6px 14px;background:#222;color:#eee;border-radius:8px;"
      "max-width:100%;display:flex;flex-wrap:wrap;gap:10px;justify-content:center;align-items:center;"
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
  }
  else
  {
    // Hidden input still present so the form structure (and the JS that
    // reads f.elements['secret']) keeps working.
    html_send_ram((char *)
      "<input type=\"hidden\" name=\"secret\" value=\"\">");
  }

  // 2) Thetis TCI panel + Band/Antenna stack, side by side, snug together.
  html_send_ram((char *)
    "<div style=\"display:flex;justify-content:center;align-items:stretch;"
    "gap:6px;flex-wrap:wrap;margin:8px auto;max-width:100%;\">");
  html_send_progmem(tci_panel);
  html_send_progmem(band_ant_panel);
  html_send_ram((char *)"</div>");

  // 4) Power & SWR.
  // Wrapped in a position:relative container so the band-mismatch warning
  // can overlay the entire section. Top row: controls column on the left
  // + K-1 (Antenna, big + warning band) on right. Bottom row: K-3 (Input)
  // and K-2 (Filter) side by side, in that order.
  emit_section("POWER &amp; SWR");
  html_send_ram((char *)
    "<div style=\"position:relative;\">"
      // Warning overlay (hidden by default). When the JS sets display=flex
      // it covers the gauges with a red banner.
      "<div id=\"tci_warn\" style=\"display:none;position:absolute;top:0;left:0;"
      "right:0;bottom:0;z-index:5;background:rgba(204,0,0,0.96);color:#fff;"
      "border-radius:10px;font-weight:bold;font-size:18px;text-align:center;"
      "padding:20px;align-items:center;justify-content:center;\"></div>"
    "<div style=\"display:flex;justify-content:center;align-items:flex-start;gap:20px;"
    "flex-wrap:wrap;max-width:100%;margin:0 auto;\">"
      "<div style=\"flex:0 0 auto;text-align:center;min-width:220px;\">");
  make_buttons();
  html_send_ram((char *)
      "</div>"
      "<div style=\"flex:1 1 auto;min-width:380px;\">");
  emit_coupler_row(1, "Power (K-1 Antenna)", "SWR (K-1 Antenna)", /*big*/1, /*warn*/1);
  html_send_ram((char *)
      "</div>"
    "</div>"
    // Second row: Input (K-3) first, Filter (K-2) second.
    "<div style=\"display:flex;justify-content:center;align-items:flex-start;gap:20px;"
    "flex-wrap:wrap;max-width:100%;margin:0 auto;\">");
  emit_coupler_row(3, "Power (K-3 Input)",   "SWR (K-3 Input)",   /*big*/0, /*warn*/0);
  emit_coupler_row(2, "Power (K-2 Filter)",  "SWR (K-2 Filter)",  /*big*/0, /*warn*/0);
  html_send_ram((char *)"</div></div>");   // close K-3/K-2 row + position:relative wrapper

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

  // 6) Trend graph — antenna power + temperature over the last 5 min.
  // Collapsible <details> so it doesn't compete for screen space when
  // not in use. JS in ajax.cpp handles sample buffering and drawing.
  html_send_ram((char *)
    "<details style=\"margin:8px auto;max-width:100%;\" open>"
      "<summary style=\"cursor:pointer;padding:6px 12px;background:#3333fa;"
                "color:white;border-radius:6px;font-weight:bold;font-size:14px;"
                "letter-spacing:1px;\">TREND (last 5 min)</summary>"
      "<div style=\"background:#fff;padding:8px;margin-top:4px;border:1px solid "
                  "#888;border-radius:6px;\">"
        "<div style=\"font-size:12px;margin-bottom:4px;\">"
          "<span style=\"color:#3333fa;font-weight:bold;\">"
            "&#9632; Antenna Power</span> &nbsp; "
          "<span style=\"color:#cc4400;font-weight:bold;\">"
            "&#9632; Temperature 1</span>"
        "</div>"
        "<canvas id=\"trendCanvas\" width=\"760\" height=\"260\" "
                "style=\"width:100%;max-width:760px;height:auto;display:block;\">"
        "</canvas>"
      "</div>"
    "</details>");

  // Close the page-spanning form
  html_send_ram((char *)"</form>");

  // 7) Setup / Config / Debug links
  html_send_ram((char *)
    "<div style=\"text-align:center;margin:14px;\">"
    "<a href=\"/setup.php\" target=\"_self\" class=\"bt\">SETUP</a> "
    "<a href=\"/config.php\" target=\"_self\" class=\"bt\">CONFIG</a> "
    "<a href=\"/tci_debug.php\" target=\"_blank\" class=\"bt\">TCI LOG</a> "
    "<a href=\"/dsp7_debug.php\" target=\"_blank\" class=\"bt\">DSP-7 LOG</a>"
    "</div>");

  // WebSocket handler keeps the entire dashboard live
  buildJavascript_control();
  html_send_ram((char *)"</body></html>");

  html_EndPage();
}
