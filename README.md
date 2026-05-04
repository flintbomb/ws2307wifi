# ws2307wifi (PlatformIO)

This is a PlatformIO conversion of the original Arduino IDE sketch
`ws2307wifi.ino` (DSP-7 Interface and Webpage, (c) 2017 Helitron Electronics).

## Building

```sh
pio run                 # compile
pio run -t upload       # flash the connected ESP8266
pio device monitor      # open serial monitor at 38400 baud
```

## OTA (over-the-air) firmware updates

The firmware includes `ArduinoOTA`, so after the first serial flash you can
update it over WiFi without disturbing the DSP-7 UART link. Useful for
in-circuit programming where the DSP-7 is connected to the same UART pins
the bootloader uses.

```sh
pio run -e esp8266_ota -t upload    # upload via WiFi
```

The OTA upload target is configured in `platformio.ini` under
`[env:esp8266_ota]`. By default it points at `10.69.69.12`; change
`upload_port` if the device's IP differs (or use `dsp7-esp.local` if mDNS
works on your network — the firmware sets that as the OTA hostname).

To require a password, uncomment the `setPassword(...)` line in `main.cpp`,
re-flash once over serial (or OTA without a password set), and uncomment
the matching `--auth=...` line in `platformio.ini`. Use the same string
in both places.

Notes:
- OTA needs roughly half the flash free to stage the new image. Current
  firmware uses ~46% so there is plenty of headroom.
- The device verifies the upload before committing it, so a failed/aborted
  OTA upload won't brick the board — it just keeps running the old image.
- OTA service uses port 8266 by default (handled internally; not the same
  as the browser-push WebSocket on port 81 or the HTTP server on port 80).

## Layout

```
ws2307wifi-pio/
├── platformio.ini         # board / framework / build flags
├── src/                   # all .cpp sources + the project header
│   ├── ws2307.h           # consolidated header (extern decls, prototypes)
│   ├── main.cpp           # was ws2307wifi.ino (setup/loop)
│   ├── HtmlHandler.cpp    # HTTP route handlers
│   ├── ajax.cpp           # AJAX/XML response builders
│   ├── config.cpp         # /config.php handler
│   ├── control.cpp        # /control.php handler
│   ├── coupler.cpp        # coupler page renderer
│   ├── debug.cpp          # *_printf helpers
│   ├── eeprom.cpp         # EEPROM persistence
│   ├── large.cpp          # main page renderer
│   ├── led.cpp            # status LED control
│   ├── multisend.cpp      # html_send_* helpers
│   ├── progmem.cpp        # PROGMEM HTML/CSS strings
│   ├── serial.cpp         # DSP-7 UART RX/TX
│   ├── setup_page.cpp     # /setup.php handler (renamed from setup.ino
│   │                      #   to avoid colliding with Arduino's setup())
│   └── ws2307_evaluate.cpp  # DSP-7 packet decoders
└── README.md
```

## Notes on the conversion

The Arduino IDE silently auto-generates forward declarations for every
function in every `.ino` file in a sketch and concatenates them into one
translation unit. PlatformIO (using vanilla g++) does not — each `.cpp`
file is compiled independently. The conversion handles this by:

1. Renaming every `.ino` to `.cpp`.
2. Adding `#include "ws2307.h"` at the top of every `.cpp`.
3. Putting every `extern` declaration and function prototype the original
   sketch relied on into `src/ws2307.h`.

The original code uses some patterns modern g++ rejects by default
(string literals assigned to non-const `char *`, functions with declared
non-void return type that don't return, etc.). `platformio.ini` enables
`-fpermissive` and silences a few related warnings so the unmodified
sources compile.

If you want to clean up those warnings later, the main offenders are:

- `wifi_printf`, `aprs_printf`, `eeprom_printf`, `wx_printf`, `html_send_ram`
  — change `char *` parameters to `const char *`.
- `make_large()`, `makeSetupHTML()` — declared returning `char *` but
  never return; change return type to `void`.
