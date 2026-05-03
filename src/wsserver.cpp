#include "ws2307.h"
#include <WebSocketsServer.h>

// Pushes a JSON state snapshot to all browser pages. Replaces the earlier
// 500ms /xml polling. Runs on a separate WebSocket port (81); HTTP keeps
// using port 80 unchanged.
//
// JSON is built into a fixed char buffer (no String allocation) so the
// 10 Hz push rate doesn't fragment the heap.

static WebSocketsServer wss(81);
static unsigned long last_push = 0;
static const unsigned long PUSH_INTERVAL_MS = 100;   // 10 Hz, matches DSP-7 update cadence

// Reusable scratch buffer. Sized for the worst case: ~17 telemetry rows of
// up to ~25 chars each, plus headers, IP, wifi, RSSI, TCI fields and JSON
// punctuation. ~1KB is the realistic max; 1536 leaves margin.
static char json_buf[1536];

// snprintf-cursor helper: append formatted text, advance cursor, never
// overrun. Returns 1 on success, 0 if the buffer was full.
#define JS_APPEND(...)                                            \
  do {                                                            \
    int _r = snprintf(json_buf + n, sizeof(json_buf) - n, __VA_ARGS__); \
    if(_r < 0) return n;                                          \
    n += _r;                                                      \
    if((size_t)n >= sizeof(json_buf)) return sizeof(json_buf) - 1;\
  } while(0)

// Append a JSON string literal value (already-quoted on each side) with
// minimal escaping for backslash and double-quote.
static int append_json_string(int n, const char *s)
{
  if((size_t)n + 1 >= sizeof(json_buf)) return n;
  json_buf[n++] = '"';
  while(*s)
  {
    char c = *s++;
    if(c == '\\' || c == '"')
    {
      if((size_t)n + 2 >= sizeof(json_buf)) break;
      json_buf[n++] = '\\';
    }
    if((size_t)n + 1 >= sizeof(json_buf)) break;
    json_buf[n++] = c;
  }
  if((size_t)n + 1 < sizeof(json_buf)) json_buf[n++] = '"';
  return n;
}

static int build_json()
{
  int n = 0;

  JS_APPEND("{\"time\":");
  n = append_json_string(n, s_acttime);
  // small space-joined date appended as separate string (saves a strcat)
  // Actually combine into one field for simpler client parsing.
  // Replace last char (close quote) with space, then date, then quote.
  if(n > 0 && json_buf[n-1] == '"')
  {
    n--;                            // drop the closing quote
    JS_APPEND(" %s\"", s_actdate);  // append space + date + closing quote
  }

  JS_APPEND(",\"ip\":");
  n = append_json_string(n, wxval[IPADDRESS].sval);

  JS_APPEND(",\"wifi\":");
  n = append_json_string(n, wxval[WIFISTATUS].sval);

  snprintf(wxval[RSSIVAL].sval, VALSTRLEN, "%ld dBm", (long)WiFi.RSSI());
  JS_APPEND(",\"rssi\":");
  n = append_json_string(n, wxval[RSSIVAL].sval);

  // table rows (DSP-7 telemetry values 0..SEPARATOR-1)
  JS_APPEND(",\"rows\":[");
  for(int i = 0; i < SEPARATOR; i++)
  {
    if(i > 0) JS_APPEND(",");
    n = append_json_string(n, t_vals[i]);
  }
  JS_APPEND("],");

  // TCI fields (Thetis state)
  JS_APPEND("\"tci_status\":\"%s\",",
            tci_connected ? "connected" : "disconnected");

  unsigned long fa = tci_vfo_a, fb = tci_vfo_b;
  JS_APPEND("\"tci_vfo_a\":\"%lu.%03lu,%03lu\",",
            fa / 1000000UL, (fa / 1000UL) % 1000UL, fa % 1000UL);
  JS_APPEND("\"tci_vfo_b\":\"%lu.%03lu,%03lu\",",
            fb / 1000000UL, (fb / 1000UL) % 1000UL, fb % 1000UL);

  JS_APPEND("\"tci_a_active\":\"%s\",", tci_a_enabled ? "(active)" : "");
  JS_APPEND("\"tci_b_active\":\"%s\",", tci_b_enabled ? "(active)" : "");
  JS_APPEND("\"tci_ptt\":\"%s\"}",       tci_ptt ? "TX" : "RX");

  return n;
}

static void on_event(uint8_t num, WStype_t type, uint8_t * /*payload*/, size_t /*length*/)
{
  if(type == WStype_CONNECTED)
  {
    int len = build_json();
    wss.sendTXT(num, (uint8_t *)json_buf, len);
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

  int len = build_json();
  wss.broadcastTXT((uint8_t *)json_buf, len);
}
