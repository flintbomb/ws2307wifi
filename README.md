# ws2307wifi (PlatformIO)

This is a PlatformIO conversion of the original Arduino IDE sketch
`ws2307wifi.ino` (DSP-7 Interface and Webpage, (c) 2017 Helitron Electronics).

## Building

```sh
pio run                 # compile
pio run -t upload       # flash the connected ESP8266
pio device monitor      # open serial monitor at 38400 baud
```

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
