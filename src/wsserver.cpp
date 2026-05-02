#include "ws2307.h"
#include <WebSocketsServer.h>

// Pushes a JSON state snapshot to all browser pages. Replaces the earlier
// 500ms /xml polling. Runs on a separate WebSocket port (81); HTTP keeps
// using port 80 unchanged.

static WebSocketsServer wss(81);
static unsigned long last_push = 0;
static const unsigned long PUSH_INTERVAL_MS = 200;   // 5 Hz

static void escape_append(String& out, const char *s)
{
  // minimal JSON string escape — backslash and quote only; the values
  // we emit don't contain control characters
  while(*s)
  {
    char c = *s++;
    if(c == '\\' || c == '"') out += '\\';
    out += c;
  }
}

static void build_json(String& out)
{
  char buf[40];

  out = "{";

  // header / status fields
  out += "\"time\":\"";
  escape_append(out, s_acttime);
  out += " ";
  escape_append(out, s_actdate);
  out += "\",";

  out += "\"ip\":\"";
  escape_append(out, wxval[IPADDRESS].sval);
  out += "\",";

  out += "\"wifi\":\"";
  escape_append(out, wxval[WIFISTATUS].sval);
  out += "\",";

  snprintf(wxval[RSSIVAL].sval, VALSTRLEN, "%ld dBm", (long)WiFi.RSSI());
  out += "\"rssi\":\"";
  escape_append(out, wxval[RSSIVAL].sval);
  out += "\",";

  // table rows (DSP-7 telemetry values 0..SEPARATOR-1)
  out += "\"rows\":[";
  for(int i = 0; i < SEPARATOR; i++)
  {
    if(i > 0) out += ",";
    out += "\"";
    escape_append(out, t_vals[i]);
    out += "\"";
  }
  out += "],";

  // TCI fields (Thetis state)
  out += "\"tci_status\":\"";
  out += tci_connected ? "connected" : "disconnected";
  out += "\",";

  unsigned long fa = tci_vfo_a, fb = tci_vfo_b;
  snprintf(buf, sizeof(buf), "%lu.%03lu,%03lu",
           fa / 1000000UL, (fa / 1000UL) % 1000UL, fa % 1000UL);
  out += "\"tci_vfo_a\":\"";
  out += buf;
  out += "\",";

  snprintf(buf, sizeof(buf), "%lu.%03lu,%03lu",
           fb / 1000000UL, (fb / 1000UL) % 1000UL, fb % 1000UL);
  out += "\"tci_vfo_b\":\"";
  out += buf;
  out += "\",";

  out += "\"tci_a_active\":\"";
  out += tci_a_enabled ? "(active)" : "";
  out += "\",";
  out += "\"tci_b_active\":\"";
  out += tci_b_enabled ? "(active)" : "";
  out += "\",";

  out += "\"tci_ptt\":\"";
  out += tci_ptt ? "TX" : "RX";
  out += "\"";

  out += "}";
}

static void on_event(uint8_t num, WStype_t type, uint8_t * /*payload*/, size_t /*length*/)
{
  if(type == WStype_CONNECTED)
  {
    String json;
    build_json(json);
    wss.sendTXT(num, json);
  }
}

void wsserver_setup()
{
  wss.begin();
  wss.onEvent(on_event);
}

void wsserver_loop()
{
  wss.loop();
  if(wss.connectedClients() == 0) return;
  if(millis() - last_push < PUSH_INTERVAL_MS) return;
  last_push = millis();

  String json;
  build_json(json);
  wss.broadcastTXT(json);
}
