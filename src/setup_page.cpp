#include "ws2307.h"

// NOTE: Renamed from setup.ino -> setup_page.cpp purely for clarity;
// the file used to live next to ws2307wifi.ino in the Arduino sketch
// folder, but inside a PlatformIO src/ tree a file named "setup.cpp"
// next to "main.cpp" reads as if it defined Arduino's setup() function.
// The actual Arduino setup() lives in main.cpp.

int ax = 0, bx = 0;

char sprache = 1;
unsigned long html_refreshtime = 10;

// send Setup Webpage to client
void handle_setupwebpage()
{
  // read data from url
  if (server.hasArg("secret"))
  {
    String s_secret = server.arg("secret");
    String s_accesscode = accesscode;

    if (s_secret == s_accesscode || apmode == 1)
    {
      if(apmode == 1)
      {
        snprintf(accesscode, 49, const_cast<char*>(s_secret.c_str()));
      }

      if (server.hasArg("ssid") && server.hasArg("password"))
      {
        String s_ssid = server.arg("ssid");
        String s_password = server.arg("password");

        if (s_ssid.length() > 0 && s_password.length() > 0)
        {
          snprintf(ssid, 49, const_cast<char*>(s_ssid.c_str()));
          snprintf(password, 49, const_cast<char*>(s_password.c_str()));
        }
      }

      if (server.hasArg("htmlinterval")) {
        String s_html_refreshtime = server.arg("htmlinterval");
        html_refreshtime = s_html_refreshtime.toInt();
      }

      if (server.hasArg("tcihost")) {
        String s_tci_host = server.arg("tcihost");
        if (s_tci_host.length() > 0) {
          snprintf(tci_host, sizeof(tci_host)-1, "%s", s_tci_host.c_str());
        }
      }
      if (server.hasArg("tciport")) {
        String s_tci_port = server.arg("tciport");
        unsigned long p = s_tci_port.toInt();
        if (p > 0 && p < 65536) tci_port = (unsigned short)p;
      }
      // tci_enabled is a checkbox: present in form -> on, absent -> off.
      // Always assigned unconditionally so unticking it actually persists.
      tci_enabled = server.hasArg("tcienabled") ? 1 : 0;

      // IP mode and static IP settings. Form always submits "ipmode" select
      // (static|dhcp), so this assignment is unconditional.
      if (server.hasArg("ipmode")) {
        use_static_ip = (server.arg("ipmode") == "static") ? 1 : 0;
      }
      if (server.hasArg("staticip")) {
        String s = server.arg("staticip");
        if (s.length() > 0) snprintf(static_ip_str, sizeof(static_ip_str)-1, "%s", s.c_str());
      }
      if (server.hasArg("staticgw")) {
        String s = server.arg("staticgw");
        if (s.length() > 0) snprintf(static_gw_str, sizeof(static_gw_str)-1, "%s", s.c_str());
      }
      if (server.hasArg("staticmask")) {
        String s = server.arg("staticmask");
        if (s.length() > 0) snprintf(static_mask_str, sizeof(static_mask_str)-1, "%s", s.c_str());
      }

      if (server.hasArg("sprache"))
      {
        sprache = 1;
      }
      else
      {
        sprache = 0;
      }

      writeEEPROM();

      makeSetupHTML();
    }
    else
    {
      server.send(404, "text/plain", "Zugang verweigert / Access denied");
    }
  }
  else
  {
    makeSetupHTML();
  }
}

// generate the HTML code of the web page
char *makeSetupHTML()
{
char text[500+1];

  html_StartPage();

  html_send_progmem(htmlpages_begin);
  html_send_progmem(setup_tit1);
  html_send_progmem(wximage);
  html_send_progmem(sprache?setup_tit2:setup_tit2_ger);
  html_send_ram(ssid);
  html_send_progmem(setup_tit3);
  html_send_progmem(sprache?setup_tit4:setup_tit4_ger);

  // IP mode + static IP settings
  snprintf(text,500,
    "<br><b><font color=\"#0000FF\">IP Configuration</font></b><br>"
    "Mode: <select name=\"ipmode\">"
    "<option value=\"static\"%s>Static</option>"
    "<option value=\"dhcp\"%s>Dynamic (DHCP)</option>"
    "</select><br>"
    "Static IP:<br>"
    "<input type=\"text\" name=\"staticip\" value=\"%s\"><br>"
    "Gateway:<br>"
    "<input type=\"text\" name=\"staticgw\" value=\"%s\"><br>"
    "Subnet Mask:<br>"
    "<input type=\"text\" name=\"staticmask\" value=\"%s\"><br>",
    use_static_ip ? " selected" : "",
    use_static_ip ? "" : " selected",
    static_ip_str, static_gw_str, static_mask_str);
  html_send_ram(text);

  // TCI server settings
  snprintf(text,500,
    "<br><b><font color=\"#0000FF\">Thetis TCI Server</font></b><br>"
    "<input type=\"checkbox\" name=\"tcienabled\" value=\"1\" %s> Enable TCI<br>"
    "Host/IP:<br>"
    "<input type=\"text\" name=\"tcihost\" value=\"%s\"><br>"
    "Port:<br>"
    "<input type=\"text\" name=\"tciport\" value=\"%u\"><br>",
    tci_enabled ? "checked" : "",
    tci_host, (unsigned)tci_port);
  html_send_ram(text);

snprintf(text,500,
"<br><input type=\"checkbox\" name=\"sprache\" value=\"1\" %s> Deutsch/English<br>\
<p><input class=\"bt\" type=\"submit\" target=\"_self\" value=\"%s\"/></p>\
<a href=\"/control.php\" target=\"_self\" class=\"bt\">%s</a><br>\
</form>\
</body>\
</html>",

  (sprache?"checked":""),
  (sprache?"SEND":"SENDEN"),
  (sprache?"RETURN":"ZUR&#220;CK")
          );

  html_send_ram(text);

  html_EndPage();

  return nullptr;
}
