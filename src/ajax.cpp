#include "ws2307.h"

String XML;
char kopplernummer = 1;
int xml_mode = 0; // 0=large, 1=coupler

const char ajax_response[] PROGMEM = " xmldoc = xmlResponse.getElementsByTagName('";
const char ajax_message[] PROGMEM = "'); message = xmldoc[0].firstChild.nodeValue; document.getElementById('largetab').rows[";
const char ajax_element[] PROGMEM = "].cells[1].innerHTML=message;";

void javascript_send_element(char *response, int row)
{
char text[10];

  html_send_progmem(ajax_response);
  html_send_ram(response);
  html_send_progmem(ajax_message);
  sprintf(text,"%d",row);
  html_send_ram(text);
  html_send_progmem(ajax_element);
}

void buildJavascript()
{
char text[20];
int rownum = 1;

  xml_mode = 0;

  html_send_progmem(XML_ScriptBegin);

  javascript_send_element((char *)"response_runtime",0);
  javascript_send_element((char *)"response_ip",19);
  javascript_send_element((char *)"response_wifi",20);
  javascript_send_element((char *)"response_signal",21);

  for(int valnum = 0; valnum < SEPARATOR; valnum++)
  {
    sprintf(text,"response_%d",rownum);
    javascript_send_element(text,rownum);

    rownum++;
  }

  html_send_progmem(XML_ScriptEnd);
}

void buildXML()
{
int rownum = 1;

  XML = "<?xml version='1.0' ?>";
  XML += "<xml>";

  if(xml_mode == 0)
  {
    // Werte fuer das grosse Display

    XML += "<response_runtime>";
    XML += s_actdate;
    XML += " ";
    XML += s_acttime;
    XML += "</response_runtime>";

    XML += "<response_ip>";
    XML += wxval[IPADDRESS].sval;
    XML += "</response_ip>";

    XML += "<response_wifi>";
    XML += wxval[WIFISTATUS].sval;
    XML += "</response_wifi>";

    sprintf(wxval[RSSIVAL].sval, "%ld dBm", WiFi.RSSI());

    XML += "<response_signal>";
    XML += wxval[RSSIVAL].sval;
    XML += "</response_signal>";

    for(int valnum = 0; valnum < SEPARATOR; valnum++)
    {
      XML += "<response_";
      XML += String(rownum);
      XML += ">";
      XML += t_vals[valnum];
      XML += "</response_";
      XML += String(rownum);
      XML += ">";
      rownum++;
    }
  }

  if(xml_mode == 1)
  {
    // Werte fuer die Kopplerdisplays

    XML += "<response_power>";
    XML += t_vals[B1_PWR + (kopplernummer - 1)*2];
    XML += "</response_power>";

    XML += "<response_swr>";
    XML += t_vals[B1_SWR + (kopplernummer - 1)*2];
    XML += "</response_swr>";
  }

  if(xml_mode == 2)
  {
    // TCI live data for the control page
    char buf[40];

    XML += "<response_tci_status>";
    XML += tci_connected ? "connected" : "disconnected";
    XML += "</response_tci_status>";

    // Format Hz as MM.kkk,hhh MHz (e.g. 14250000 -> "14.250,000")
    unsigned long fa = tci_vfo_a;
    unsigned long fb = tci_vfo_b;
    snprintf(buf, sizeof(buf), "%lu.%03lu,%03lu",
             fa / 1000000UL, (fa / 1000UL) % 1000UL, fa % 1000UL);
    XML += "<response_tci_vfo_a>";
    XML += buf;
    XML += "</response_tci_vfo_a>";

    snprintf(buf, sizeof(buf), "%lu.%03lu,%03lu",
             fb / 1000000UL, (fb / 1000UL) % 1000UL, fb % 1000UL);
    XML += "<response_tci_vfo_b>";
    XML += buf;
    XML += "</response_tci_vfo_b>";

    XML += "<response_tci_a_active>";
    XML += tci_a_enabled ? "(active)" : "";
    XML += "</response_tci_a_active>";

    XML += "<response_tci_b_active>";
    XML += tci_b_enabled ? "(active)" : "";
    XML += "</response_tci_b_active>";

    XML += "<response_tci_ptt>";
    XML += tci_ptt ? "TX" : "RX";
    XML += "</response_tci_ptt>";
  }

  XML += "</xml>";
}

const char ajax_resp_pwr[] PROGMEM = " xmldoc = xmlResponse.getElementsByTagName('response_power'); message = xmldoc[0].firstChild.nodeValue; document.getElementById('runtime_power').innerHTML=message;";
const char ajax_resp_swr[] PROGMEM = " xmldoc = xmlResponse.getElementsByTagName('response_swr'); message = xmldoc[0].firstChild.nodeValue; document.getElementById('runtime_swr').innerHTML=message;";

void buildJavascript_coupler(char coupnum)
{
  xml_mode = 1;
  kopplernummer = coupnum;

  html_send_progmem(XML_ScriptBegin);

  html_send_progmem(ajax_resp_pwr);
  html_send_progmem(ajax_resp_swr);

  html_send_progmem(XML_ScriptEnd);
}

static void send_tci_js_field(const char *xmltag, const char *spanid)
{
  char buf[300];
  snprintf(buf, sizeof(buf),
    " xmldoc = xmlResponse.getElementsByTagName('%s'); "
    "if(xmldoc.length){ message = xmldoc[0].firstChild ? xmldoc[0].firstChild.nodeValue : ''; "
    "document.getElementById('%s').innerHTML=message; }",
    xmltag, spanid);
  html_send_ram(buf);
}

void buildJavascript_control()
{
  xml_mode = 2;

  html_send_progmem(XML_ScriptBegin);

  send_tci_js_field("response_tci_status",   "tci_status");
  send_tci_js_field("response_tci_vfo_a",    "tci_vfo_a");
  send_tci_js_field("response_tci_vfo_b",    "tci_vfo_b");
  send_tci_js_field("response_tci_a_active", "tci_a_active");
  send_tci_js_field("response_tci_b_active", "tci_b_active");
  send_tci_js_field("response_tci_ptt",      "tci_ptt");

  html_send_progmem(XML_ScriptEnd);
}
