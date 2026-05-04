#include "ws2307.h"

// this string is used to prepare the full HTML page
// (tests with sending in fragments did not work with some firefox versions)

// ======================================================
// if the client calls an URL not existing on this server
// ======================================================
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void handleRoot()
{
  // The old large-table page was retired — / now renders the unified
  // dashboard from control.cpp. handle_control() takes care of the
  // optional secret/passcode arg for the action buttons.
  handle_control();
}


