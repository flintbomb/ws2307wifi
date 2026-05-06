/*
 * DSP-7 Interface and Webpage: Copyright (c) 2017, Helitron Electronics www.helitron.de
 *
 * All rights reserved
 *
 * PlatformIO conversion: was ws2307wifi.ino
 */

#include "ws2307.h"
#include <ArduinoOTA.h>

// personal unique access code
char accesscode[50] = {"1234"};
unsigned char require_passcode = 1;   // setup-page checkbox; persisted to EEPROM

char callsign[15] = "";

// WiFi login data
char ssid[50] = {"DropItLikeUtaAHotspot_IoT"};
char password[50] = {"flintbomb"};

// Static IP configuration. Defaults to static 10.69.69.12 in STA mode;
// can be overridden via the setup page (saved to EEPROM).
unsigned char use_static_ip   = 1;
char          static_ip_str[16]   = STATIC_IP;
char          static_gw_str[16]   = "10.69.69.1";
char          static_mask_str[16] = "255.255.255.0";

char apmode = 0;

// this local WebServer
ESP8266WebServer server(80);

/* ===========================================
 *  SETUP: called once after Reset
 *  ==========================================
 */
void setup ( void )
{
unsigned char conncnt = 0;

  // configure the LED outputs
  init_leds();

  readEEPROM();
  //test_eeprom();

  // setup the serial UART for communication with DSP-7
  Serial.begin ( 38400 );

  // Start WiFi immediately. AP-mode detection runs from loop() during
  // the first 5 seconds so it doesn't block this setup path.
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  if(use_static_ip)
  {
    IPAddress ip, gw, mask;
    if(ip.fromString(static_ip_str) && gw.fromString(static_gw_str) && mask.fromString(static_mask_str))
      WiFi.config(ip, gw, mask);
  }
  WiFi.begin(ssid, password);
  MDNS.begin("esp8266");
  WiFi.softAPdisconnect(true);
  apmode = 0;

  pinMode(0, INPUT_PULLUP);   // GPIO0 polled in loop() for AP-mode request

  // setup the WebServer
  // set the functions called if a user requests this webpage or subpage
  server.on ("/", handleRoot );
  server.on ("/setup.php",handle_setupwebpage);
  server.on ("/control.php",handle_control);
  server.on ("/config.php",handle_config);
  server.on ("/tci_debug.php", handle_tci_debug);
  server.on ("/dsp7_debug.php", handle_dsp7_debug);

  // if not existing page is called
  server.onNotFound ( handleNotFound );

  // start the Webserver
  server.begin();
  // HTTP server started

  // start the WebSocket server (browser push) and TCI WebSocket client
  wsserver_setup();
  if(!apmode) tci_setup();

  // Over-the-Air firmware updates. After the first serial flash, future
  // uploads can target the device's IP/hostname over WiFi without touching
  // the DSP-7 serial link. Configure password if exposed to untrusted
  // networks; for an isolated lab network the default (no password) is fine.
  ArduinoOTA.setHostname("dsp7-esp");
  // ArduinoOTA.setPassword("changeme");
  ArduinoOTA.begin();

  //testvals(); // !!!!!!!!!!!!1 TEST ONLY
}

/*
 * MAIN LOOP: running forever// check if Jumper is set, if yes, enter AP mode
 */
void loop ( void )
{
static char connstat = 0;
static char ap_check_done = 0;

  // Non-blocking AP-mode detection: poll GPIO0 for the first 5 seconds.
  if(!ap_check_done && !apmode)
  {
    if(digitalRead(0) == LOW)
    {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_AP);
      WiFi.softAP("WS23SETUP");
      apmode = 1;
      wifi_printf((char *)"interner AP aktiviert. SSID=WS23SETUP, IP=192.168.4.1");
      led_wlan_connected(2);
    }
    if(millis() > 5000) ap_check_done = 1;
  }

  if(apmode)
  {
    server.handleClient();
    led_wlan_connected(2);
    return;
  }

  if(WiFi.status() == WL_CONNECTED)
  {
    led_wlan_connected(1);
    if(!connstat) wifi_printf((char *)"connected to %s",ssid);
    connstat = 1;
  }
  else
  {
    led_wlan_connected(0);
    if(connstat) wifi_printf((char *)"lost connection from %s",ssid);
    connstat = 0;
  }

  // handle client requests to this WebInterface
  server.handleClient();

  // if data received, process it immediately
  ws_getdata();

  // send Trigger for dynamic port routes (i.e. for Speedport Router)
  udp_trigger();

  // send to DSP-7
  dsp7_send();

  // service TCI WebSocket (client) and browser push WebSocket (server)
  tci_loop();
  wsserver_loop();

  // service OTA (no-op unless an upload is in progress)
  ArduinoOTA.handle();
}

/*
 * some routers have a dymamic port routing
 * they trigger to outgoing data on a specific port
 * and then route incoming data (on other ports) to this device
 *
 * send some dummy data to somewhere, use port 49899 as trigger port
 * do this after reset and every 10 minutes
 */
void udp_trigger()
{
const char *trigger_ip = "8.8.8.8"; // dummy, use any IP
int trigger_port = 49901;
static long lastsend = 0;

  if((millis()-lastsend)>(10*60*1000))
  //if((millis()-lastsend)>(2000))
  {
    lastsend = millis();

    WiFiUDP udp;

    udp.begin(49899);
    udp.beginPacket(trigger_ip,trigger_port);
    char buf[1] = {0};
    udp.write(buf,1);
    udp.endPacket();
    udp.stop();
  }
}
