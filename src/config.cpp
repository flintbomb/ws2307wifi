#include "ws2307.h"

int handle_config()
{
  // read data from url
  if (server.hasArg("secret"))
  {
    String s_secret = server.arg("secret");
    String s_config = server.arg("CONFIG");
    String s_accesscode = accesscode;

    if (s_secret == s_accesscode)
    {
      // "Refresh" button: re-send the 0x07 request to make the DSP-7 push
      // its config dump, then poll the UART for ~1s so the reply arrives
      // before we re-render the page.
      if(server.hasArg("REFRESH"))
      {
        unsigned char tx[4] = { 0x01, 0x02, 0x03, 0x07 };
        Serial.write(tx, 4);

        unsigned long start = millis();
        while(millis() - start < 1500)
        {
          ws_getdata();   // consume bytes as they arrive
          delay(1);
        }
        makeConfigHTML(s_secret);
        return 1;
      }

      if(s_config.length() < 100)
        makeConfigHTML(s_secret);
      else
      {
        send_cfg_to_DSP7(s_config);
        handleRoot();
      }

      return 1;
    }
    else
    {
      server.send(404, "text/plain", "Zugang verweigert / Access denied");
    }
  }
  else
  {
    makeConfigHTML("");
  }
  return 0;
}


const char config_tit1[] PROGMEM = R"=====(
<!doctype html><head><meta name="viewport" content="width=device-width, initial-scale=1"><title>DSP-7 CONFIG</title><style>
body{background-color:#cccccc;font-family:Arial,Helvetica,Sans-Serif;Color:#000088;}

.greentitle{
padding:8px;
margin-bottom:8px;
background-color:#3333fa;
font-size:22px;
font-weight:bold;
border-radius:12px;
color:white;
}

.bt{
background-color:#3333fa;
border:none;
color:white;
padding:6px 20px;
text-align:center;
text-decoration:none;
display:inline-block;
font-size:16px;
font-weight:bold;
margin:4px 2px;
cursor:pointer;
border-radius:12px;
}

p {
font-family: verdana;
font-weight:bold;
font-size: 24px;
}
}
</style>
<script type="text/javascript">

function saveTextAsFile()
{
    var textToSave = document.getElementById("inputTextToSave").value;
    var textToSaveAsBlob = new Blob([textToSave], {type:"text/plain"});
    var textToSaveAsURL = window.URL.createObjectURL(textToSaveAsBlob);
    var fileNameToSaveAs = document.getElementById("inputFileNameToSaveAs").value;

    var downloadLink = document.createElement("a");
    downloadLink.download = fileNameToSaveAs;
    downloadLink.innerHTML = "Download File";
    downloadLink.href = textToSaveAsURL;
    downloadLink.onclick = destroyClickedElement;
    downloadLink.style.display = "none";
    document.body.appendChild(downloadLink);

    downloadLink.click();
}

function destroyClickedElement(event)
{
    document.body.removeChild(event.target);
}

function loadFileAsText()
{
    var fileToLoad = document.getElementById("fileToLoad").files[0];

    var fileReader = new FileReader();
    fileReader.onload = function(fileLoadedEvent)
    {
        var textFromFileLoaded = fileLoadedEvent.target.result;
        document.getElementById("outputTextToSave").value = textFromFileLoaded;
    };
    fileReader.readAsText(fileToLoad, "UTF-8");
}

</script>
</head>
)=====";

const char download_config_eng[] PROGMEM = R"=====(
<br><b><font color="#0000FF" size="6">SAVE as FILE</font><br>
<br><b><font color="#0000FF">save the DSP-7 configuration as a file<br><br>
)=====";



const char download_config_ger[] PROGMEM = R"=====(
<br><b><font color="#0000FF" size="6">SPEICHERE als DATEI</font><br>
<br><b><font color="#0000FF">Speichere die Konfiguration von DSP-7 als Datei<br><br>
)=====";


const char download_config[] PROGMEM = R"=====(
<br><input type="hidden" id="inputFileNameToSaveAs">
<br><button class="bt" onclick="saveTextAsFile()">SAVE</button><br><br><hr><br>
)=====";

const char upload_config_eng[] PROGMEM = R"=====(
<br><b><font color="#0000FF" size="6">UPLOAD</font><br>
<br><b><font color="#0000FF">UPLOAD the configuration from a file into DSP-7<br>
<br>* Click "BROWSE",
<br>* choose the folder and configuration file
<br>* and click "Open"
<br>* enter your PASSCODE
<br>* Click "UPLOAD" to send it into DSP-7<br><br>
)=====";

const char upload_config_ger[] PROGMEM = R"=====(
<br><b><font color="#0000FF" size="6">UPLOAD</font><br>
<br><b><font color="#0000FF">Lade eine Konfigurationsdatei zum DSP-7 hoch<br>
<br>* Klicke "DURCHSUCHEN",
<br>* waehle ein Verzeichnis und die Konfigurationsdatei
<br>* und Klicke "Oeffnen"
<br>* gebe den korrekten PASSCODE ein
<br>* Klicke "UPLOAD" um sie zu DSP-7 zu senden<br><br>
)=====";


const char upload_config1[] PROGMEM = R"=====(
<input class="bt" type="file" id="fileToLoad" onchange="loadFileAsText()">  <!-- Select Configuration file for Upload -->
<form>
<br><br><b><font color="#0000FF">PASSCODE:
)=====";

const char upload_config2[] PROGMEM = R"=====(
Conf-Data: <input  name="CONFIG" id="outputTextToSave" readonly> <!-- Config for DSP7 -->
<br><br><button class="bt" type="submit" name="Upload">UPLOAD</button>
</form>
<br><hr><br><br><a href="/" target="_self" class="bt">RETURN</a><br>
</body></html>
)=====";


// ---- Decoded config table ----------------------------------------------
//
// stm32_config[] holds the raw 216-byte packet from the DSP-7:
//   [0..3]   header 01 02 03 05
//   [4..5]   length field (big-endian, = 0x00D6 = 214)
//   [6..213] t_fdata payload (208 bytes, little-endian / IEEE-754 LE)
//   [214..215] CRC16
//
// Field map below mirrors the t_fdata struct documented in the firmware
// notes. Reserved / dummy / touch-calibration fields are deliberately not
// surfaced.

#define CFG_PAYLOAD_OFFSET 6

static uint16_t cfg_u16(int payload_off)
{
  unsigned char *p = stm32_config + CFG_PAYLOAD_OFFSET + payload_off;
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t cfg_u32(int payload_off)
{
  unsigned char *p = stm32_config + CFG_PAYLOAD_OFFSET + payload_off;
  return (uint32_t)p[0]
       | ((uint32_t)p[1] << 8)
       | ((uint32_t)p[2] << 16)
       | ((uint32_t)p[3] << 24);
}

static uint8_t cfg_u8(int payload_off)
{
  return stm32_config[CFG_PAYLOAD_OFFSET + payload_off];
}

static float cfg_f32(int payload_off)
{
  uint32_t v = cfg_u32(payload_off);
  float f;
  memcpy(&f, &v, 4);
  return f;
}

static void cfg_row_u(const char *name, unsigned long val, const char *unit)
{
  char buf[160];
  snprintf(buf, sizeof(buf),
    "<tr><td>%s</td><td>%lu</td><td>%s</td></tr>",
    name, val, unit ? unit : "");
  html_send_ram(buf);
}

static void cfg_row_hex32(const char *name, uint32_t val)
{
  char buf[160];
  snprintf(buf, sizeof(buf),
    "<tr><td>%s</td><td>0x%08lX</td><td></td></tr>",
    name, (unsigned long)val);
  html_send_ram(buf);
}

static void cfg_row_f(const char *name, float val, const char *unit)
{
  char buf[160];
  snprintf(buf, sizeof(buf),
    "<tr><td>%s</td><td>%.3f</td><td>%s</td></tr>",
    name, (double)val, unit ? unit : "");
  html_send_ram(buf);
}

static void cfg_row_str(const char *name, const char *val)
{
  char buf[200];
  snprintf(buf, sizeof(buf),
    "<tr><td>%s</td><td colspan=\"2\">%s</td></tr>",
    name, val);
  html_send_ram(buf);
}

static void make_config_table()
{
  // Need a full packet: 4 header + 2 length + 208 payload + 2 CRC = 216
  if (stm32_cfglen < 216) return;

  html_send_ram((char *)
    "<style>"
    ".cfgtbl{border-collapse:collapse;margin:8px 0;background:#fff;font-size:13px;}"
    ".cfgtbl th,.cfgtbl td{border:1px solid #888;padding:3px 8px;text-align:left;}"
    ".cfgtbl th{background:#3333fa;color:#fff;}"
    ".cfgtbl tr.sec td{background:#dde;font-weight:bold;}"
    "</style>"
    "<table class=\"cfgtbl\">"
    "<tr><th>Field</th><th>Value</th><th>Unit</th></tr>");

  // --- header section ---
  html_send_ram((char *)"<tr class=\"sec\"><td colspan=\"3\">Header</td></tr>");
  cfg_row_hex32("magic", cfg_u32(0));

  // --- protection limits ---
  html_send_ram((char *)"<tr class=\"sec\"><td colspan=\"3\">Protection Limits</td></tr>");
  cfg_row_u("I_limit",            cfg_u16(20), "A");
  cfg_row_u("U_limit",            cfg_u16(22), "V");
  cfg_row_u("Temp_limit",         cfg_u16(24), cfg_u8(56) ? "&deg;F" : "&deg;C");
  cfg_row_u("Drive_limit",        cfg_u16(26), "W");
  cfg_row_u("Time_limit_minutes", cfg_u16(28), "min");
  cfg_row_u("fan_temp",           cfg_u16(30), cfg_u8(56) ? "&deg;F" : "&deg;C");
  cfg_row_u("fan_speed",          cfg_u16(32), "%");

  // --- system settings ---
  html_send_ram((char *)"<tr class=\"sec\"><td colspan=\"3\">System Settings</td></tr>");
  {
    uint8_t v = cfg_u8(54);
    cfg_row_str("language", v == 0 ? "0 (German)" : v == 1 ? "1 (English)" : "?");
  }
  cfg_row_str("display_reverse", cfg_u8(55) ? "1 (flipped)" : "0 (normal)");
  cfg_row_str("tempunits",       cfg_u8(56) ? "1 (&deg;F)" : "0 (&deg;C)");
  cfg_row_u("maxpwrunits",   cfg_u8(57), "");
  cfg_row_u("fanonpttunits", cfg_u8(58), "");
  cfg_row_u("maxUunits",     cfg_u8(59), "");
  cfg_row_u("maxIunits",     cfg_u8(60), "");
  cfg_row_u("bandmode",      cfg_u8(61), "");
  cfg_row_u("shuntR",        cfg_u8(62), "");
  cfg_row_str("systemMode",   cfg_u8(63) ? "1 (Pwr/SWR Meter)" : "0 (PA Controller)");
  cfg_row_str("auxInputMode", cfg_u8(64) ? "1 (4xPTT inputs)"  : "0 (Rotary Switch)");
  cfg_row_u("civ_adr1", cfg_u8(65), "");
  cfg_row_u("civ_adr2", cfg_u8(66), "");
  cfg_row_u("civ_adr3", cfg_u8(67), "");
  cfg_row_u("civ_adr4", cfg_u8(68), "");

  // --- RF power calibration ---
  html_send_ram((char *)"<tr class=\"sec\"><td colspan=\"3\">RF Power Calibration</td></tr>");
  cfg_row_f("ant_W_low",   cfg_f32(80),  "W");
  cfg_row_f("ant_W_high",  cfg_f32(84),  "W");
  cfg_row_f("ant_mV_low",  cfg_f32(88),  "mV");
  cfg_row_f("ant_mV_high", cfg_f32(92),  "mV");
  cfg_row_f("flt_W_low",   cfg_f32(96),  "W");
  cfg_row_f("flt_W_high",  cfg_f32(100), "W");
  cfg_row_f("flt_mV_low",  cfg_f32(104), "mV");
  cfg_row_f("flt_mV_high", cfg_f32(108), "mV");
  cfg_row_f("drv_W_low",   cfg_f32(112), "W");
  cfg_row_f("drv_W_high",  cfg_f32(116), "W");
  cfg_row_f("drv_mV_low",  cfg_f32(120), "mV");
  cfg_row_f("drv_mV_high", cfg_f32(124), "mV");

  // --- antenna switch ---
  html_send_ram((char *)"<tr class=\"sec\"><td colspan=\"3\">Antenna Switch</td></tr>");
  cfg_row_u("antsw (current)", cfg_u8(168), "");
  for (int ant = 0; ant < 3; ant++)
  {
    char row[256];
    int n = snprintf(row, sizeof(row),
      "<tr><td>antsw_bandsel[ant%d][b1..b11]</td><td colspan=\"2\">", ant + 1);
    for (int band = 0; band < 11; band++)
    {
      n += snprintf(row + n, sizeof(row) - n, "%s%u",
                    band ? ", " : "",
                    (unsigned)cfg_u8(169 + ant * 11 + band));
    }
    snprintf(row + n, sizeof(row) - n, "</td></tr>");
    html_send_ram(row);
  }

  // --- struct integrity ---
  html_send_ram((char *)"<tr class=\"sec\"><td colspan=\"3\">Integrity</td></tr>");
  cfg_row_hex32("crc16 (struct)", cfg_u32(204));

  html_send_ram((char *)"</table>");
}

void makeConfigHTML(String s_secret)
{
char text[500+1];
char cfg[500+1];

  html_StartPage();

  html_send_progmem(config_tit1);
  html_send_ram((char *)"<body><p><a class=\"greentitle\"><img width=\"32\" height=\"25\" src=\"");
  html_send_progmem(wximage);
  if(!sprache)
    html_send_ram((char *)"\" alt=\" \"/>  DSP-7 KONFIGURATION</a></p>");
  else
    html_send_ram((char *)"\" alt=\" \"/>  DSP-7 CONFIG</a></p>");
  html_send_progmem(sprache?download_config_eng:download_config_ger);

  int len = getConfig(cfg);
  if(len == 0) strcpy(cfg,"no Cfg Data");
  if(len > 495) strcpy(cfg,"cfg too long");
  snprintf(text,500,"Conf-Data: <input  id=\"inputTextToSave\" value=\"%s\" readonly>",cfg);
  html_send_ram(text);

  make_config_table();

  // "Refresh" button — re-fetches the config from the DSP-7 by sending
  // command 0x07. Useful when the DSP-7 has been power-cycled separately
  // or its local config changed since the ESP came up. Passcode required.
  snprintf(text, 500,
    "<br><b><font color=\"#0000FF\">PASSCODE:</font></b> "
    "<form style=\"display:inline\">"
    "<input type=\"text\" name=\"secret\" value=\"%s\" size=\"10\">"
    "<input type=\"hidden\" name=\"REFRESH\" value=\"1\">"
    "<button class=\"bt\" type=\"submit\">REFRESH FROM DSP-7</button>"
    "</form><br><br>",
    const_cast<char*>(s_secret.c_str()));
  html_send_ram(text);

  html_send_progmem(download_config);

  html_send_progmem(sprache?upload_config_eng:upload_config_ger);
  html_send_progmem(upload_config1);
  snprintf(text,200,"<input type=\"text\" name=\"secret\" value=\"%s\"><br><br>",const_cast<char*>(s_secret.c_str()));
  html_send_ram(text);
  html_send_progmem(upload_config2);

  html_EndPage();
}
