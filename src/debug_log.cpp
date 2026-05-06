// Lightweight ring-buffer logs for TCI (text) and DSP-7 (binary) traffic.
// Each log holds the last DBG_SIZE entries; pages auto-refresh once per
// second to show new traffic. Direction is 'R' (received from peer) or
// 'T' (transmitted to peer).

#include "ws2307.h"

#define DBG_LEN  160
#define DBG_SIZE 32

typedef struct {
  unsigned long ms;
  char          dir;
  char          text[DBG_LEN];
} dbg_t;

static dbg_t tci_log[DBG_SIZE];
static dbg_t dsp_log[DBG_SIZE];
static unsigned int tci_head = 0;   // next write slot (also the oldest entry)
static unsigned int dsp_head = 0;

static void put_text(dbg_t *log, unsigned int *head,
                     char dir, const char *msg, size_t len)
{
  dbg_t *e = &log[*head];
  e->ms = millis();
  e->dir = dir;
  if (len >= DBG_LEN) len = DBG_LEN - 1;
  memcpy(e->text, msg, len);
  e->text[len] = 0;
  *head = (*head + 1) % DBG_SIZE;
}

static void put_hex(dbg_t *log, unsigned int *head,
                    char dir, const unsigned char *data, size_t len)
{
  dbg_t *e = &log[*head];
  e->ms = millis();
  e->dir = dir;
  size_t maxbytes = (DBG_LEN - 5) / 3;   // "XX " per byte + room for "..."
  size_t actual   = len > maxbytes ? maxbytes : len;
  char *p = e->text;
  for (size_t i = 0; i < actual; i++) p += sprintf(p, "%02X ", data[i]);
  if (actual < len) p += sprintf(p, "...");
  *p = 0;
  *head = (*head + 1) % DBG_SIZE;
}

void debug_tci_log(char dir, const char *msg, size_t len)
{
  put_text(tci_log, &tci_head, dir, msg, len);
}

void debug_dsp7_log(char dir, const unsigned char *data, size_t len)
{
  put_hex(dsp_log, &dsp_head, dir, data, len);
}

static void render_log(const char *title, dbg_t *log, unsigned int head)
{
  char buf[400];

  html_StartPage();
  html_send_ram((char *)
    "<!doctype html><head>"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<meta http-equiv=\"refresh\" content=\"1\">"
    "<title>");
  html_send_ram((char *)title);
  html_send_ram((char *)"</title><style>"
    "body{background:#111;color:#cfc;font-family:monospace;font-size:13px;margin:8px;}"
    "h1{color:#0fb;font-size:16px;margin:4px 0;}"
    "a{color:#fc8;text-decoration:none;}"
    "a:hover{text-decoration:underline;}"
    ".rx{color:#7fe07f;} .tx{color:#ffc080;}"
    "table{width:100%;border-collapse:collapse;}"
    "td{padding:2px 6px;vertical-align:top;border-bottom:1px solid #222;}"
    "td.t{color:#888;width:90px;white-space:nowrap;}"
    "td.d{width:24px;font-weight:bold;text-align:center;}"
    "td.m{word-break:break-all;}"
    ".bar{margin-bottom:6px;display:flex;gap:14px;flex-wrap:wrap;}"
    "</style></head><body>"
    "<div class=\"bar\"><h1>");
  html_send_ram((char *)title);
  html_send_ram((char *)"</h1>"
    "<a href=\"/control.php\">&larr; Control</a>"
    "<a href=\"/tci_debug.php\">TCI</a>"
    "<a href=\"/dsp7_debug.php\">DSP-7</a>"
    "<span style=\"color:#666;\">auto-refresh 1s</span>"
    "</div>"
    "<table>");

  // Walk from oldest (head, i.e. next write slot) to newest.
  for (int i = 0; i < DBG_SIZE; i++)
  {
    unsigned int j = (head + i) % DBG_SIZE;
    if (log[j].ms == 0) continue;     // unused slot
    const char *cls = (log[j].dir == 'R') ? "rx" : "tx";
    snprintf(buf, sizeof(buf),
      "<tr><td class=\"t\">%lu ms</td>"
      "<td class=\"d %s\">%c</td>"
      "<td class=\"m %s\">%s</td></tr>",
      log[j].ms, cls, log[j].dir, cls, log[j].text);
    html_send_ram(buf);
  }

  html_send_ram((char *)"</table></body></html>");
  html_EndPage();
}

void handle_tci_debug()  { render_log("TCI Debug",   tci_log, tci_head); }
void handle_dsp7_debug() { render_log("DSP-7 Debug", dsp_log, dsp_head); }
