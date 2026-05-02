#include "ws2307.h"
#include <WebSocketsClient.h>

char           tci_host[40] = "10.69.69.3";
unsigned short tci_port     = 50001;
unsigned long  tci_vfo_a    = 0;
unsigned long  tci_vfo_b    = 0;
unsigned char  tci_a_enabled = 1;       // VFO A defaults to on (TCI sends events as state changes)
unsigned char  tci_b_enabled = 0;
unsigned char  tci_ptt      = 0;
unsigned char  tci_connected = 0;

static WebSocketsClient ws;
static unsigned long last_reconnect_attempt = 0;

// TCI messages are semicolon-terminated text frames. A single WS payload
// may contain several. Parse each command:value list.
static void parse_tci_line(const char *line)
{
  // vfo:<rx>,<vfo>,<freq>;
  if(strncmp(line, "vfo:", 4) == 0)
  {
    int rx = -1, vfo = -1;
    unsigned long freq = 0;
    if(sscanf(line + 4, "%d,%d,%lu", &rx, &vfo, &freq) == 3 && rx == 0)
    {
      if(vfo == 0) tci_vfo_a = freq;
      else if(vfo == 1) tci_vfo_b = freq;
    }
    return;
  }

  // trx:<rx>,<true|false>;
  if(strncmp(line, "trx:", 4) == 0)
  {
    tci_ptt = strstr(line, "true") ? 1 : 0;
    return;
  }

  // rx_channel_enable:<rx>,<channel>,<true|false>;
  // channel 0 = VFO A, channel 1 = sub-RX (VFO B)
  if(strncmp(line, "rx_channel_enable:", 18) == 0)
  {
    int rx = -1, ch = -1;
    if(sscanf(line + 18, "%d,%d", &rx, &ch) == 2 && rx == 0)
    {
      unsigned char on = strstr(line, "true") ? 1 : 0;
      if(ch == 0) tci_a_enabled = on;
      else if(ch == 1) tci_b_enabled = on;
    }
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
      ws.sendTXT("start;");
      break;
    case WStype_DISCONNECTED:
      tci_connected = 0;
      break;
    case WStype_TEXT:
      parse_tci_payload((const char *)payload, length);
      break;
    default:
      break;
  }
}

void tci_setup()
{
  if(tci_host[0] == 0 || tci_port == 0) return;
  ws.begin(tci_host, tci_port, "/");
  ws.onEvent(ws_event);
  ws.setReconnectInterval(5000);
}

void tci_loop()
{
  ws.loop();
}
