#include "ws2307.h"

// data received from the dsp7 are stored here
unsigned char ws_rxdata[RXSTRLEN];

// read data from dsp7
void ws_getdata()
{
int d,i;

  if(!Serial.available()) return;

  d = Serial.read();
  if(d == -1) return;


  // Daten wurden empfangen
  // schreibe das empfangene Byte in ws_rxdata
  // schiebe alles bisher empfangene nach vorn
  for(i=0; i<(RXSTRLEN-1); i++)
    ws_rxdata[i] = ws_rxdata[i+1];

  ws_rxdata[RXSTRLEN-1] = d;

  // der Datensatz ist komplett wenn vorne der Header drin steht
  if(ws_rxdata[0] == 0x1e &&
    ws_rxdata[1] == 0x7c &&
    ws_rxdata[2] == 0xfe &&
    ws_rxdata[3] == 0x56)
    {
      evaluate_DSP7data0(ws_rxdata);
      debug_dsp7_log('R', ws_rxdata, 64);
    }

    if(ws_rxdata[0] == 0x64 &&
    ws_rxdata[1] == 0x78 &&
    ws_rxdata[2] == 0xab &&
    ws_rxdata[3] == 0x61)
    {
      evaluate_DSP7data1(ws_rxdata);
      debug_dsp7_log('R', ws_rxdata, 64);
    }

    if(ws_rxdata[0] == 0x01 &&
    ws_rxdata[1] == 0x02 &&
    ws_rxdata[2] == 0x03 &&
    ws_rxdata[3] == 0x05)
    {
      evaluate_DSP7data2(ws_rxdata);
      debug_dsp7_log('R', ws_rxdata, 64);
    }
}

char switchDSP7_off;
char switchDSP7_on;
char switchDSP7_standby;
char switchDSP7_active;

void dsp_sendcontrol()
{
unsigned char tx[5];
static int first = 1;

  tx[0] = 0;

  if(txcfg_len != 0)
  {
    Serial.write(txcfg,txcfg_len);
    debug_dsp7_log('T', txcfg, txcfg_len);
    txcfg_len = 0;
    return;
  }

  if(first)
  {
    // Nach dem Programmstart melde dich bei DSP-7
    first = 0;

    tx[0] = 1;
    tx[1] = 2;
    tx[2] = 3;
    tx[3] = 7;
    Serial.write(tx,4);
    debug_dsp7_log('T', tx, 4);
    return;
  }

  if(switchDSP7_off == 1)
  {
    switchDSP7_off = 0;
    tx[0] = 1;
    tx[1] = 2;
    tx[2] = 3;
    tx[3] = 1;
  }
  if(switchDSP7_on == 1)
  {
    switchDSP7_on = 0;
    tx[0] = 1;
    tx[1] = 2;
    tx[2] = 3;
    tx[3] = 2;
  }
  if(switchDSP7_standby == 1)
  {
    switchDSP7_standby = 0;
    tx[0] = 1;
    tx[1] = 2;
    tx[2] = 3;
    tx[3] = 3;
  }
  if(switchDSP7_active == 1)
  {
    switchDSP7_active = 0;
    tx[0] = 1;
    tx[1] = 2;
    tx[2] = 3;
    tx[3] = 4;
  }
  if(tx[0] != 0)
  {
    Serial.write(tx,4);
    debug_dsp7_log('T', tx, 4);
  }
}

void ws_sendIP()
{
unsigned char tx[4];

  strcpy(wxval[IPADDRESS].sval,const_cast<char*>(WiFi.localIP().toString().c_str()));

  tx[0] = 1;
  tx[1] = 2;
  tx[2] = 3;
  tx[3] = 6;
  Serial.write(tx,4);
  Serial.write(wxval[IPADDRESS].sval,20);
  debug_dsp7_log('T', tx, 4);
}

// Send command 0x08: TX frequency update for DSP-7 band selection.
// 8-byte frame, no CRC: 01 02 03 08 [B3][B2][B1][B0]  (freq Hz, big-endian).
// DSP-7 times out stale data after 3 s; we send every 500 ms while a freq
// is known, plus a single zero frame when the source drops out so it can
// fall back to CIV/band-voltage immediately instead of waiting the timeout.
static void send_tx_freq(unsigned long freq)
{
  unsigned char tx[8];
  tx[0] = 0x01; tx[1] = 0x02; tx[2] = 0x03; tx[3] = 0x08;
  tx[4] = (freq >> 24) & 0xFF;
  tx[5] = (freq >> 16) & 0xFF;
  tx[6] = (freq >>  8) & 0xFF;
  tx[7] =  freq        & 0xFF;
  Serial.write(tx, 8);
  debug_dsp7_log('T', tx, 8);
}

void dsp7_send()
{
static long lastsend = 0;
static long last_freqsend = 0;
static unsigned long last_freq_sent = 0;

  // sende Web Kommandos
  dsp_sendcontrol();

  // sende eigene lokale IP alle 2s
  if((millis()-lastsend)>(2*1000))
  {
    lastsend = millis();
    ws_sendIP();
  }

  // TX frequency update — every 500 ms while we have one. If the source
  // drops (tci_tx_freq goes to 0), emit one final zero frame so the DSP-7
  // doesn't have to wait its 3 s timeout to release the override.
  if((millis() - last_freqsend) > 500)
  {
    last_freqsend = millis();
    if(tci_enabled && tci_tx_freq != 0)
    {
      send_tx_freq(tci_tx_freq);
      last_freq_sent = tci_tx_freq;
    }
    else if(last_freq_sent != 0)
    {
      // Source dropped (TCI disabled or freq cleared) — push one zero
      // frame so the DSP-7 releases the override immediately instead of
      // waiting its 3 s timeout.
      send_tx_freq(0);
      last_freq_sent = 0;
    }
  }
}
