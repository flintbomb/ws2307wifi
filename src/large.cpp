#include "ws2307.h"

/*
   Original Webseite, formatiert
*/

char *titles_deutsch_pwrswr[MAXVALUES] =
{
  (char *)"Betriebsstatus", // OPSTATE = 0,
  (char *)"PTT (PA)", // PTT_OUT,
  (char *)"Band", // BAND_SELECTED,
  (char *)"Antenne", // ANTENNA_SELECTED,
  (char *)"Temperatur-1", // TEMPERATURE1,
  (char *)"Temperatur-2", // TEMPERATURE2,
  (char *)"L&#252;fterstatus", // FAN,
  (char *)"DC Spannung", // DCVOLT,
  (char *)"DC Strom", // DCAMP,
  (char *)"DC Leistung", // DCPWR,
  (char *)"Wirkungsgrad", // EFFICIENCY,
  (char *)"K-1: Ausgangsleistung", // B1_PWR,
  (char *)"K-1: SWR",              // B1_SWR,
  (char *)"K-2: Ausgangsleistung", // B2_PWR,
  (char *)"K-2: SWR",              // B2_SWR,
  (char *)"K-3: Ausgangsleistung", // B3_PWR,
  (char *)"K-3: SWR",              // B3_SWR,
  (char *)"",              // Separator
  (char *)"lokale IP Adresse", // IPADDRESS,
  (char *)"WiFi Status", // WIFISTATUS,
  (char *)"Signalst&#228;rke" // RSSIVAL,
};

char *titles_deutsch_pa[MAXVALUES] =
{
  (char *)"Betriebsstatus", // OPSTATE = 0,
  (char *)"PTT (PA)", // PTT_OUT,
  (char *)"Band", // BAND_SELECTED,
  (char *)"Antenne", // ANTENNA_SELECTED,
  (char *)"Temperatur-1", // TEMPERATURE1,
  (char *)"Temperatur-2", // TEMPERATURE2,
  (char *)"L&#252;fterstatus", // FAN,
  (char *)"DC Spannung", // DCVOLT,
  (char *)"DC Strom", // DCAMP,
  (char *)"DC Leistung", // DCPWR,
  (char *)"Wirkungsgrad", // EFFICIENCY,
  (char *)"Ausgangsleistung", // B1_PWR,
  (char *)"Antennen-SWR",     // B1_SWR,
  (char *)"Filter Vorlauf", // B2_PWR,
  (char *)"Filter-SWR",              // B2_SWR,
  (char *)"Eingangsleistung", // B3_PWR,
  (char *)"Eingangs-SWR",              // B3_SWR,
  (char *)"",
  (char *)"lokale IP Adresse", // IPADDRESS,
  (char *)"WiFi Status", // WIFISTATUS,
  (char *)"Signalst&#228;rke" // RSSIVAL,
};


char *titles_english_pwrswr[MAXVALUES] =
{
  (char *)"Operating State", // OPSTATE = 0,
  (char *)"PTT (PA)", // PTT_OUT,
  (char *)"Band", // BAND_SELECTED,
  (char *)"Antenna", // ANTENNA_SELECTED,
  (char *)"Temperature-1", // TEMPERATURE1,
  (char *)"Temperature-2", // TEMPERATURE2,
  (char *)"Fan state", // FAN,
  (char *)"DC Voltage", // DCVOLT,
  (char *)"DC Current", // DCAMP,
  (char *)"DC Power", // DCPWR,
  (char *)"Efficiency", // EFFICIENCY,
  (char *)"K-1: Output Power", // B1_PWR,
  (char *)"K-1: SWR",              // B1_SWR,
  (char *)"K-2: Output Power", // B2_PWR,
  (char *)"K-2: SWR",              // B2_SWR,
  (char *)"K-3: Output Power", // B3_PWR,
  (char *)"K-3: SWR",              // B3_SWR,
  (char *)"",
  (char *)"local IP Address", // IPADDRESS,
  (char *)"WiFi Status", // WIFISTATUS,
  (char *)"Signal strength" // RSSIVAL,
};

char *titles_english_pa[MAXVALUES] =
{
  (char *)"Operating State", // OPSTATE = 0,
  (char *)"PTT (PA)", // PTT_OUT,
  (char *)"Band", // BAND_SELECTED,
  (char *)"Antenna", // ANTENNA_SELECTED,
  (char *)"Temperature-1", // TEMPERATURE1,
  (char *)"Temperature-2", // TEMPERATURE2,
  (char *)"Fan state", // FAN,
  (char *)"DC Voltage", // DCVOLT,
  (char *)"DC Current", // DCAMP,
  (char *)"DC Power", // DCPWR,
  (char *)"Efficiency", // EFFICIENCY,
  (char *)"Output Power", // B1_PWR,
  (char *)"Antenna SWR",              // B1_SWR,
  (char *)"Filter Forward", // B2_PWR,
  (char *)"Filter SWR",              // B2_SWR,
  (char *)"Input Power", // B3_PWR,
  (char *)"Input SWR",              // B3_SWR,
  (char *)"",
  (char *)"local IP Address", // IPADDRESS,
  (char *)"WiFi Status", // WIFISTATUS,
  (char *)"Signal strength" // RSSIVAL,
};

char *units[MAXVALUES] =
{
  (char *)"", // OPSTATE = 0,
  (char *)"", // PTT_OUT,
  (char *)"", // BAND_SELECTED,
  (char *)"", // ANTENNA_SELECTED,
  (char *)"degrees", // TEMPERATURE1,
  (char *)"degrees", // TEMPERATURE2,
  (char *)"", // FAN,
  (char *)"V", // DCVOLT,
  (char *)"A", // DCAMP,
  (char *)"W", // DCPWR,
  (char *)"%", // EFFICIENCY,
  (char *)"W",
  (char *)":1",
  (char *)"W",
  (char *)":1",
  (char *)"W",
  (char *)":1",
  (char *)"",
  (char *)"",
  (char *)"",
  (char *)""
};

// ================================================================================

// Start des Headers und Refreshintervall
void make_largeheader()
{
  html_send_progmem(htmlpages_begin);
  html_send_progmem(largepage_css_style);

  buildJavascript();

  html_send_ram((char *)"<body onload='process()'>");
  html_send_progmem(html_picture);
  html_send_ram((char *)"<table id=\"largetab\">");
}

void insert_3column_line(char *title, char *c1, char *c2)
{
  char text[100];

  snprintf(text,100, "<tr><td>%s</td><td>%s</td><td colspan=\"2\">%s</td></tr>", title, c1, c2);
  html_send_ram(text);
}

void insert_2column_line(char *title, char *c1, char *c2)
{
  char text[100];

  snprintf(text,100, "<tr><td>%s</td><td colspan=\"3\">%s%s</td></tr>", title, c1, c2);
  html_send_ram(text);
}


void insert_datatable()
{
  int i;
  char fahrenheit[20] = "&#176; F";
  char celsius[20] = "&#176; C";
  char text[100];

  // Uptime
  snprintf(text,100, "<tr><td>%s</td><td colspan=\"3\">--- ---</td></tr>", (char *)(sprache ? "Uptime" : "Laufzeit"));
  html_send_ram(text);

  // die Tabelleneintraege
  for (i = 0; i < MAXVALUES; i++)
  {
    if (i == SEPARATOR)
      html_send_progmem(html_lsep);

    else if (i == TEMPERATURE1 || i == TEMPERATURE2)
      insert_3column_line((sysmode ? (sprache ? titles_english_pwrswr[i] : titles_deutsch_pwrswr[i]) : (sprache ? titles_english_pa[i] : titles_deutsch_pa[i])), (char *)"---"/*t_vals[i]*/, tempunits ? fahrenheit : celsius);

    else if (i > SEPARATOR)
      insert_2column_line((sysmode ? (sprache ? titles_english_pwrswr[i] : titles_deutsch_pwrswr[i]) : (sprache ? titles_english_pa[i] : titles_deutsch_pa[i])), (char *)"---" /*wxval[i].sval*/, (char *)" ");

    else
      insert_3column_line((sysmode ? (sprache ? titles_english_pwrswr[i] : titles_deutsch_pwrswr[i]) : (sprache ? titles_english_pa[i] : titles_deutsch_pa[i])), (char *)"---"/*t_vals[i]*/, units[i]);
  }
}

char *make_large()
{
  strcpy(wxval[IPADDRESS].sval, const_cast<char*>(WiFi.localIP().toString().c_str()));
  sprintf(wxval[WIFISTATUS].sval, "AP: %s", ssid);

  // Header mit Titel, CSS und Bildertitel
  make_largeheader();

  // Datentabelle
  insert_datatable();

  // Tail
  html_send_progmem(sprache ? html_ltail : html_ltail_german);

  return nullptr;
}
