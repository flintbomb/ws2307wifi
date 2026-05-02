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


void makeControlHTML(String s_secret)
{
char text[400+1];

  html_StartPage();

  html_send_progmem(control_tit1);
  html_send_progmem(wximage);
  html_send_progmem(sprache?control_tit2:control_tit2_ger);

  snprintf(text,100,"%s\"><br><br>",const_cast<char*>(s_secret.c_str()));
  html_send_ram(text);

  make_buttons();
  html_send_ram((char *)"</form>");

  // TCI live status panel
  snprintf(text, 400,
    "<div style=\"margin-top:16px;padding:10px;background:#f4f4f4;"
    "border:1px solid #888;border-radius:8px;font-family:monospace;\">"
    "<b>Thetis TCI</b><br>"
    "Status: <span id=\"tci_status\">--</span><br>"
    "VFO A: <span id=\"tci_vfo_a\">--</span> MHz "
    "<span id=\"tci_a_active\"></span><br>"
    "VFO B: <span id=\"tci_vfo_b\">--</span> MHz "
    "<span id=\"tci_b_active\"></span><br>"
    "PTT: <span id=\"tci_ptt\">--</span>"
    "</div>");
  html_send_ram(text);

  // Inject the AJAX script that polls /xml in control mode and updates
  // the TCI display fields above. Then kick it off.
  buildJavascript_control();
  html_send_ram((char *)"<script>process();</script></body></html>");

  html_EndPage();
}
