#ifndef WS2307_H_
#define WS2307_H_

// =====================================================================
// Consolidated header for the ws2307wifi project.
//
// In the original Arduino IDE sketch every .ino file could see every
// other .ino file's globals and functions for free, because Arduino
// concatenates them into one translation unit and auto-generates
// forward declarations. PlatformIO compiles each .cpp independently, so
// this header collects all the cross-file declarations the sketch
// relied on. Every src/*.cpp begins with `#include "ws2307.h"`.
// =====================================================================

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// ----------------- enums shared across files --------------------------

enum {
  VALUE1 = 0,
  VALUE2,
  PHASE,
  LASTVAL,
  ARROWDIR,
  MAX_SIZE
};

// List of all values measured, calculated or just displayed in the html table
enum {
  // Status
  OPSTATE = 0,
  PTT,

  // Band
  BAND_SELECTED,

  // Antenna
  ANTENNA_SELECTED,

  // Temperature
  TEMPERATURE1,
  TEMPERATURE2,
  FAN,

  // Power-Supply
  DCVOLT,
  DCAMP,
  DCPWR,
  EFFICIENCY,

  // Bridge-1
  B1_PWR,
  B1_SWR,

  // Bridge-2
  B2_PWR,
  B2_SWR,

  // Bridge-3
  B3_PWR,
  B3_SWR,

  // separator
  SEPARATOR,

  // Debug data
  IPADDRESS,
  WIFISTATUS,
  RSSIVAL,

  MAXVALUES
};

// ----------------- shared types ---------------------------------------

#define VALSTRLEN  25
#define LASTVALANZ 10
#define RXSTRLEN   300

typedef struct {
  short ival;                       // actual value in 1/10 steps
  short lastvals[LASTVALANZ];       // remember the last N values
  char  valid;                      // 1 = first valid value received
  short prevval;                    // previous value for tendency
  char  arrowdir;                   // 1 = up, 2 = down
  char  sval[VALSTRLEN];            // printable string
} WXVALS;

typedef struct {
  long fwd_dBm;
  long fwd_dBmpeak;
  long fwd_watt;
  long fwd_peakwatt;
  long rev_dBm;
  long rev_watt;
  long swr;
  long imp_min;
  long imp_max;
  long refl_attenuation;
} t_pwrswr;

// ----------------- LED pin definitions --------------------------------

#define LED_GN  12
#define LED_BL  5
#define LED_RD  4

#define LEDON  0
#define LEDOFF 1

// ----------------- globals defined in main.cpp ------------------------

extern char accesscode[50];
extern char callsign[15];
extern char ssid[50];
extern char password[50];
extern unsigned char use_static_ip;
extern char          static_ip_str[16];
extern char          static_gw_str[16];
extern char          static_mask_str[16];
extern char apmode;
extern ESP8266WebServer server;

// ----------------- globals defined in serial.cpp ----------------------

extern unsigned char ws_rxdata[RXSTRLEN];
extern char switchDSP7_off;
extern char switchDSP7_on;
extern char switchDSP7_standby;
extern char switchDSP7_active;

// ----------------- globals defined in eeprom.cpp ----------------------

extern int dx;
extern int reserved_eeprom_size;
extern unsigned long ee_magic;

// ----------------- globals defined in setup_page.cpp ------------------

extern int ax, bx;
extern char sprache;
extern unsigned long html_refreshtime;

// ----------------- globals defined in tci.cpp -------------------------

extern char           tci_host[40];
extern unsigned short tci_port;
extern unsigned long  tci_vfo_a;
extern unsigned long  tci_vfo_b;
extern unsigned char  tci_a_enabled;    // 1 if RX channel 0 (VFO A) enabled
extern unsigned char  tci_b_enabled;    // 1 if RX channel 1 (VFO B / sub-RX) enabled
extern unsigned char  tci_ptt;          // 0 = RX, 1 = TX
extern unsigned char  tci_connected;    // 1 if WS connected

// ----------------- globals defined in debug.cpp -----------------------

extern char aprs_debmsg[200];
extern char wifi_msg[100];
extern char eeprom_msg[100];
extern char wx_msg[100];

// ----------------- globals defined in ajax.cpp ------------------------

extern String XML;
extern char kopplernummer;
extern int  xml_mode;

// ----------------- globals defined in ws2307_evaluate.cpp -------------

extern WXVALS wxval[MAXVALUES];
extern char   t_vals[MAXVALUES][25];
extern char   s_acttime[30];
extern char   s_actdate[11];
extern unsigned char *prxdata;
extern unsigned char  sysmode;
extern unsigned char  state;
extern unsigned char  antsw;
extern long           band;
extern unsigned char  ptt;
extern unsigned char  fan;
extern unsigned char  tempunits;
extern unsigned char  bandmode;
extern long           uptime;
extern t_pwrswr       pwrswr_input;
extern t_pwrswr       pwrswr_filter;
extern t_pwrswr       pwrswr_antenna;
extern unsigned char  stm32_config[500];
extern int            stm32_cfglen;
extern unsigned char  txcfg[300];
extern int            txcfg_len;

// ----------------- PROGMEM strings defined in progmem.cpp -------------

extern const char XML_ScriptBegin[]     PROGMEM;
extern const char XML_ScriptEnd[]       PROGMEM;
extern const char htmlpages_begin[]     PROGMEM;
extern const char largepage_css_style[] PROGMEM;
extern const char html_picture[]        PROGMEM;
extern const char html_BodyBegin[]      PROGMEM;
extern const char html_ltail[]          PROGMEM;
extern const char html_ltail_german[]   PROGMEM;
extern const char html_lsep[]           PROGMEM;
extern const char wximage[]             PROGMEM;
extern const char setup_tit1[]          PROGMEM;
extern const char setup_tit2[]          PROGMEM;
extern const char setup_tit2_ger[]      PROGMEM;
extern const char setup_tit3[]          PROGMEM;
extern const char setup_tit4[]          PROGMEM;
extern const char setup_tit4_ger[]      PROGMEM;

// ----------------- function prototypes --------------------------------

// main.cpp
void udp_trigger();

// led.cpp
void init_leds();
char APmodeRequested();
void led_wlan_connected(char onoff);
void led_dataRXed(char mode);
void led_access_eeprom(char onoff);

// debug.cpp
void aprs_printf(char *fmt, ...);
void wifi_printf(char *fmt, ...);
void eeprom_printf(char *fmt, ...);
void wx_printf(char *fmt, ...);

// multisend.cpp
void html_StartPage();
void html_send_ram(char *data);
void html_send_progmem(PGM_P data);
void html_EndPage();

// HtmlHandler.cpp
void handleNotFound();
void handleRoot();
void handleCoup1();
void handleCoup2();
void handleCoup3();
void handleXML();

// ajax.cpp
void javascript_send_element(char *response, int row);
void buildJavascript();
void buildXML();
void buildJavascript_coupler(char coupnum);

// config.cpp
int  handle_config();
void makeConfigHTML(String s_secret);

// control.cpp
int  handle_control();
void make_buttons();
void makeControlHTML(String s_secret);

// coupler.cpp
void make_coupler(char coupnum);

// large.cpp
void  make_largeheader();
void  insert_3column_line(char *title, char *c1, char *c2);
void  insert_2column_line(char *title, char *c1, char *c2);
void  insert_datatable();
char *make_large();

// setup_page.cpp
void  handle_setupwebpage();
char *makeSetupHTML();

// tci.cpp
void tci_setup();
void tci_loop();
void buildJavascript_control();

// eeprom.cpp
void ee_begin();
void ee_endread();
void ee_endwrite();
void writeEEPROM();
char readEEPROM();
void ee_writeChecksum();
char ee_readChecksum();

// serial.cpp
void ws_getdata();
void dsp_sendcontrol();
void ws_sendIP();
void dsp7_send();

// ws2307_evaluate.cpp
unsigned int crc16_bytecalc(unsigned char byte);
unsigned int crc16_messagecalc(unsigned char *data, int len);
int  checkData(unsigned char *ws_rxdata);
void get32bit(long *pv);
void get8bit(unsigned char *pv);
void getpwrswr(void *pt);
void print32(char *ps, long v);
void printuptime();
void evaluate_DSP7data0(unsigned char *ws_rxdata);
void evaluate_DSP7data1(unsigned char *ws_rxdata);
void evaluate_DSP7data2(unsigned char *ws_rxdata);
char convbcd(unsigned char d);
int  getConfig(char *pres);
unsigned char BCD_to_HEX(char high, char low);
void send_cfg_to_DSP7(String s_config);

#endif // WS2307_H_
