#include "ws2307.h"
#include <WebSocketsClient.h>

char           tci_host[40] = "10.69.69.3";
unsigned short tci_port     = 50001;
unsigned char  tci_enabled  = 1;
unsigned long  tci_vfo_a    = 0;
unsigned long  tci_vfo_b    = 0;
unsigned long  tci_tx_freq  = 0;
unsigned char  tci_a_enabled = 1;       // resolved by recompute_active_rx()
unsigned char  tci_b_enabled = 0;
unsigned char  tci_ptt      = 0;
unsigned char  tci_connected = 0;

// Thetis TCI sends tx_frequency_ex with no RX index attached. To know
// which VFO the transmitter is actually using (so we can later forward
// the band selection to the DSP-7) we compare the reported TX frequency
// against each receiver's main VFO and pick whichever is closer. This
// also drives the (active) badge in the dashboard.
static unsigned long abs_diff(unsigned long a, unsigned long b)
{
  return a > b ? a - b : b - a;
}

static void recompute_active_rx()
{
  // Until we have both a TX freq and at least one VFO, leave RX1 marked
  // active so the UI shows something sensible on cold start.
  if (tci_tx_freq == 0) return;

  if (tci_vfo_a != 0 && tci_vfo_b == 0) {
    tci_a_enabled = 1; tci_b_enabled = 0; return;
  }
  if (tci_vfo_a == 0 && tci_vfo_b != 0) {
    tci_a_enabled = 0; tci_b_enabled = 1; return;
  }
  if (tci_vfo_a == 0 && tci_vfo_b == 0) return;

  if (abs_diff(tci_vfo_a, tci_tx_freq) <= abs_diff(tci_vfo_b, tci_tx_freq)) {
    tci_a_enabled = 1; tci_b_enabled = 0;
  } else {
    tci_a_enabled = 0; tci_b_enabled = 1;
  }
}

static WebSocketsClient ws;
static unsigned long last_reconnect_attempt = 0;

// TCI messages are semicolon-terminated text frames. A single WS payload
// may contain several. Parse each command:value list.
static void parse_tci_line(const char *line)
{
  // vfo:<rx>,<vfo>,<freq>;  — track main VFO (vfo 0) of each receiver:
  // rx 0 -> tci_vfo_a (displayed as "RX1"), rx 1 -> tci_vfo_b ("RX2").
  // Active-RX is decided by recompute_active_rx(), not dial motion.
  if(strncmp(line, "vfo:", 4) == 0)
  {
    int rx = -1, vfo = -1;
    unsigned long freq = 0;
    if(sscanf(line + 4, "%d,%d,%lu", &rx, &vfo, &freq) == 3 && vfo == 0)
    {
      if(rx == 0) tci_vfo_a = freq;
      else if(rx == 1) tci_vfo_b = freq;
      recompute_active_rx();
    }
    return;
  }

  // tx_frequency_ex:<freq>;  — Thetis broadcasts the active TX frequency
  // without a receiver index. We pin which RX it belongs to by matching
  // it against tci_vfo_a / tci_vfo_b. Stored in tci_tx_freq for later
  // forwarding to the DSP-7 for band selection.
  if(strncmp(line, "tx_frequency_ex:", 16) == 0)
  {
    unsigned long freq = 0;
    if(sscanf(line + 16, "%lu", &freq) == 1)
    {
      tci_tx_freq = freq;
      recompute_active_rx();
    }
    return;
  }

  // trx:<rx>,<true|false>;
  if(strncmp(line, "trx:", 4) == 0)
  {
    tci_ptt = strstr(line, "true") ? 1 : 0;
    return;
  }

}

static void parse_tci_payload(const char *payload, size_t len)
{
  char buf[128];
  size_t bi = 0;
  for(size_t i = 0; i < len; i++)
  {
    char c = payload[i];
    if(c == ';' || c == '\n' || c == '\r')
    {
      if(bi > 0)
      {
        buf[bi] = 0;
        parse_tci_line(buf);
        bi = 0;
      }
    }
    else if(bi < sizeof(buf) - 1)
    {
      buf[bi++] = c;
    }
  }
}

static void ws_event(WStype_t type, uint8_t *payload, size_t length)
{
  switch(type)
  {
    case WStype_CONNECTED:
      tci_connected = 1;
      debug_tci_log('I', "[connected]", 11);
      ws.sendTXT("start;");
      debug_tci_log('T', "start;", 6);
      break;
    case WStype_DISCONNECTED:
      tci_connected = 0;
      debug_tci_log('I', "[disconnected]", 14);
      break;
    case WStype_TEXT:
      debug_tci_log('R', (const char *)payload, length);
      parse_tci_payload((const char *)payload, length);
      break;
    default:
      break;
  }
}

void tci_setup()
{
  if(!tci_enabled) return;
  if(tci_host[0] == 0 || tci_port == 0) return;
  ws.begin(tci_host, tci_port, "/");
  ws.onEvent(ws_event);
  ws.setReconnectInterval(5000);
}

void tci_loop()
{
  if(!tci_enabled)
  {
    // Make sure stale TCI state can't drive band selection on the DSP-7
    // after the user disables it.
    tci_connected = 0;
    tci_tx_freq   = 0;
    return;
  }
  ws.loop();
}
