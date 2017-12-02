#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>




/* Set these to your desired credentials. */
const char *ssid = "WATER_LEVEL_DISPLAY";
const char *password = "waterleveldisplay";
const int ledcount = 9;
const int motor_pin = 3;
const int blink_pin = 0;

ESP8266WebServer server ( 80 );

static const uint8_t  gpio[ledcount] = {D0, D1, D2, D3, D4, D5, D6, D7, D8};

int level = 0;
boolean dataIndicator = false;
boolean high_water = false;
boolean prev_high_water = false;
boolean low_water = false;
boolean prev_low_water = false;


void handleRoot() {
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf ( temp, 400,

"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>Water Level Display</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from Water Level Display!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    </body>\
</html>",

    hr, min % 60, sec % 60
  );
  server.send ( 200, "text/html", temp );
}

void setGPIO() {
    int state = LOW;
    if (server.arg("n")== ""){     //Parameter not found
      Serial.println("Parameter 'n' (normalized water level) not specified");
      server.send(200, "text/html", "<H3>Parameter 'n' (normalized water level) not specified</H3>");
    } else {     //Parameter found
      level = (server.arg("n")).toInt(); 

      for(int i=0; i< ledcount; i++ ) {
        if(i < level) {
          digitalWrite ( gpio[i], 1 );
          if(i == blink_pin) dataIndicator = true;
          if(i == ledcount - 1) {
            prev_high_water = high_water;
            high_water = true;
            if(!prev_high_water) { 
              Serial.println("Prev High Level was LOW, now HIGH, so turning OFF motor_pin"); 
              digitalWrite ( motor_pin, LOW ); 
            }
          }
          if(i == 0) {
            prev_low_water = low_water;
            low_water = true;
          }          
        } else {
          digitalWrite ( gpio[i], 0 );
          if(i == blink_pin) dataIndicator = false;
          prev_high_water = high_water;
          if(i == ledcount - 1) {
            prev_high_water = high_water;
            high_water = false;
          }
          if(i == 0) {
            prev_low_water = low_water;
            low_water = false;
            if(prev_low_water) {
              Serial.println("Prev Low Level was HIGH, now LOW, so turning ON motor_pin"); 
              digitalWrite ( motor_pin, HIGH ); 
            }
          }
          
        }
      }
      
      Serial.print("level set to ");
      Serial.print(level);
      Serial.print("  high_water:");
      Serial.print(high_water);
      Serial.print("  prev_high_water:");
      Serial.print(prev_high_water);
      Serial.print("  low_water:");
      Serial.print(low_water);
      Serial.print("  prev_low_water:");
      Serial.println(prev_low_water);

      
      
      server.send(200, "text/html", "<H3>level set to " + String(level) + "</H3>");

      if(dataIndicator == false) state = HIGH;
      else state = LOW;
      digitalWrite(gpio[blink_pin], state);
      
    }
    if(dataIndicator) state = HIGH;
    else state = LOW;
    delay(200);
    digitalWrite(gpio[blink_pin], state);
}


void debug() {
  digitalWrite(blink_pin, not dataIndicator);
  if (server.arg("m")== ""){     //Parameter not found
    Serial.println("Parameter 'm' (debug message) not specified");
    server.send(200, "text/html", "<H3>Parameter 'm' (debug message) not specified</H3>");
  } else {     //Parameter found
    Serial.println("DEBUG: " + server.arg("m"));
    server.send(200, "text/html", "OK");
  }
  digitalWrite(blink_pin, not dataIndicator);
}

void handleNotFound() {
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



void setup() {

  delay(1000);
  Serial.begin(115200);
  Serial.println();
  pinMode(motor_pin, FUNCTION_3);
  pinMode(motor_pin, OUTPUT);
  
  for(int i=0; i< ledcount; i++ ) {
    pinMode ( gpio[i], OUTPUT );
    digitalWrite ( gpio[i], 0 );
  }

  digitalWrite ( motor_pin, LOW );

  Serial.println ( "" );

  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  if ( MDNS.begin ( "esp8266" ) ) {
    Serial.println ( "MDNS responder started" );
  }

  server.on ( "/", handleRoot );
  server.on ( "/set", setGPIO );
  server.on ( "/debug", debug );
  server.on ( "/inline", []() {
    server.send ( 200, "text/plain", "this works as well" );
  } );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );
}

void loop() {
    server.handleClient();
}

