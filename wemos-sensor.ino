#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"         //https://github.com/tzapu/WiFiManager
#include <FS.h>  
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>


std::unique_ptr<ESP8266WebServer> server;

const int trigger = D2;  // the GPIO that is connected to the Trigger Pin of the Ultrasound Sensor
const int echo = D1;     // the GPIO that is connected to the Echo Pin of the Ultrasound Sensor
char tank_bottom_distance[6] = "150";  // default
char max_water_level[6] = "120";              // default
char speed_of_sound[6] = "342";          // default, m/s
bool shouldSaveConfig = false;
long previous_level = 0;
long distance_to_water = 0;
long duration = 0;
int fakeCount = 0;
const int ledcount = 9;

HTTPClient http;

// webserver callback
void handleRoot() {
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf ( temp, 400,

"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>Water Level Sensor</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from Water Level Sensor!</h1>\
    <br>\
    Config:  /config <br> \
    Get Config: /getconfig <br>\
    distance_to_water:  /d <br>\
    speed_of_sound: /sets?s=<<speed_of_sound>> <br>\
    max_water_level: /setm?m=<<ma_water_level>> <br>\
    tank_bottom_distance: /sett?t=<<tank_bottom_distance>> <br>\
    <p>Uptime: %02d:%02d:%02d</p>\
    </body>\
</html>",

    hr, min % 60, sec % 60
  );
  server->send ( 200, "text/html", temp );
}

// webserver callback
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";
  for (uint8_t i = 0; i < server->args(); i++) {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  server->send(404, "text/plain", message);
}


//callback notifying us of the need to save config

void saveConfigCallback () {
  Serial.println("saveConfigCallback: Should save config");
  shouldSaveConfig = true;
}


//reads config parameters from SPIFFS file-system and sets global vars

int readSpiffsConfig() {
    Serial.println("readSpiffsConfig: mounting FS...");
    if (SPIFFS.begin()) {
      Serial.println("readSpiffsConfig: mounted file system");
      if (SPIFFS.exists("/config.json")) {
        //file exists, reading and loading
        Serial.println("readSpiffsConfig: reading config file");
        File configFile = SPIFFS.open("/config.json", "r");
        if (configFile) {
          Serial.println("readSpiffsConfig: opened config file");
          size_t size = configFile.size();
          // Allocate a buffer to store contents of the file.
          std::unique_ptr<char[]> buf(new char[size]);
  
          configFile.readBytes(buf.get(), size);
          DynamicJsonBuffer jsonBuffer;
          JsonObject& json = jsonBuffer.parseObject(buf.get());
          
          if (json.success()) {
            Serial.print("\nreadSpiffsConfig: parsed json from Config File: ");
            json.printTo(Serial);
            if(json["speed_of_sound"]) strcpy(speed_of_sound, json["speed_of_sound"]);
            if(json["tank_bottom_distance"]) strcpy(tank_bottom_distance, json["tank_bottom_distance"]);
            if(json["max_water_level"]) strcpy(max_water_level, json["max_water_level"]);
            Serial.print("readSpiffsConfig: Config From SPIFFS: speed_of_sound = ");
            Serial.println(speed_of_sound);
            Serial.print("readSpiffsConfig: Config From SPIFFS: tank_bottom_distance  = ");
            Serial.println(tank_bottom_distance);
            Serial.print("readSpiffsConfig: Config From SPIFFS: max_water_level    = ");
            Serial.println(max_water_level);

          } else {
            Serial.println("readSpiffsConfig: failed to load json config");
          }
          configFile.close();
        }
      }
    } else {
      Serial.println("readSpiffsConfig: failed to mount FS");
    }
    return 0;
  }


// Function to try and connect to Wi-Fi using previously saved credentials.
// If no saved creds are found, it starts an Access Point and a web-server
// which can be used to connect to any scanned Wi-Fi SSID. The UI also allows
// the configuration of 'speed of sound', 'mqtt server (host)' and 'mqtt port' 
 
void wifiManagerSetup(WiFiManager& wifiManager) {
  WiFiManagerParameter custom_speed_of_sound("speedofsound", "speed of sound", speed_of_sound, 6);
  WiFiManagerParameter custom_tank_bottom_distance("tankbottomdistance", "tank bottom distance", tank_bottom_distance, 6);
  WiFiManagerParameter custom_max_water_level("maxwaterlevel", "max water level", max_water_level, 6);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_speed_of_sound);
  wifiManager.addParameter(&custom_tank_bottom_distance);
  wifiManager.addParameter(&custom_max_water_level);
  Serial.println("wifiManagerSetup: Trying AutoConnect ...");
  wifiManager.autoConnect("WATER_LEVEL_SENSOR");
  strcpy(speed_of_sound, custom_speed_of_sound.getValue());
  strcpy(tank_bottom_distance, custom_tank_bottom_distance.getValue());
  strcpy(max_water_level, custom_max_water_level.getValue());
  if(shouldSaveConfig)
    Serial.print("wifiManagerSetup: Config From User: speed_of_sound = ");
  else
    Serial.print("wifiManagerSetup: Config From SPIFFS: speed_of_sound = ");  
  Serial.println(speed_of_sound);
  if(shouldSaveConfig)
    Serial.print("wifiManagerSetup: Config From User: tank_bottom_distance = ");
  else
    Serial.print("wifiManagerSetup: Config From SPIFFS: tank_bottom_distance = ");  
  Serial.println(tank_bottom_distance);
  if(shouldSaveConfig)
    Serial.print("wifiManagerSetup: Config From User: max_water_level   = ");
  else
    Serial.print("wifiManagerSetup: Config From SPIFFS: max_water_level   = ");  
  Serial.println(max_water_level);
}


// Function to save config paraeters from global vars to SPIFFS as a JSON

void saveConfig() {
  Serial.println("saveConfig: Saving Config to SPIFFS");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["speed_of_sound"] = speed_of_sound;
  json["tank_bottom_distance"] = tank_bottom_distance;
  json["max_water_level"] = max_water_level;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("saveConfig: failed to open config file for writing");
  }
  Serial.print("saveConfig: JSON to be saved: ");
  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
  Serial.println("\nsaveConfig: Config saved to SPIFFS");

}

// Creates a web-server while being connected to the Wi-Fi router,
// exposing a "/config" URL to manually reset the saved Wi-Fi creds 
// and re-configure the same.
// "/sets", "/setp", and "/seth" enables configuring parameters manually
// "/showconfig" displays current values of configuration 

void createWebServer() {
  server.reset(new ESP8266WebServer(WiFi.localIP(), 80));

  server->on("/", handleRoot);

  server->on("/inline", []() {
    server->send(200, "text/html", "<H2>this works as well</H2>");
  });

  server->on("/config", []() {
    server->send(200, "text/html", "<H3>Connect to WATER_LEVEL_SENSOR and browse the IP address 192.168.4.1 to Setup the Water Level Sensor</H3>");
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    server.reset();
    Serial.println("createWebServer (/config): Starting Local HotSpot at 192.168.4.1 for Configuation...");
    wifiManagerSetup(wifiManager);
    Serial.println("\ncreateWebServer (/config): Configuation Done.");
    saveConfig();    
    server.reset(new ESP8266WebServer(WiFi.localIP(), 80));
    Serial.println("createWebServer (/config): Connected to Router: IP = " + WiFi.localIP());

  });

  server->on("/sets", []() {
    if (server->arg("s")== ""){     //Parameter not found
      Serial.println("Parameter 's' (speed_of_sound) not specified");
      server->send(200, "text/html", "<H3>Parameter 's' (speed_of_sound) not specified</H3>");
    } else {     //Parameter found
      (server->arg("s")).toCharArray(speed_of_sound, 1 + (server->arg("s")).length()); 
      Serial.println("speed_of_sound set to " + String(speed_of_sound));
      server->send(200, "text/html", "<H3>speed_of_sound set to " + String(speed_of_sound) + "</H3>");
      saveConfig();
    }
  });
 server->on("/setm", []() {
    if (server->arg("m")== ""){     //Parameter not found
      Serial.println("Parameter 'm' (max water level) not specified");
      server->send(200, "text/html", "<H3>Parameter 'm' (max water level) not specified</H3>");
    } else {     //Parameter found
      (server->arg("m")).toCharArray(max_water_level, 1 + (server->arg("m")).length()); 
      Serial.println("max_water_level set to " + String(max_water_level));
      server->send(200, "text/html", "<H3>max_water_level set to " + String(max_water_level) + "</H3>");
      saveConfig();
    }
  });
 server->on("/sett", []() {
    if (server->arg("t")== ""){     //Parameter not found
      Serial.println("Parameter 't' (tank bottom distance) not specified");
      server->send(200, "text/html", "<H3>Parameter 't' (tank bottom distance) not specified</H3>");
    } else {     //Parameter found
      (server->arg("t")).toCharArray(tank_bottom_distance, 1 + (server->arg("t")).length()); 
      Serial.println("tank_bottom_distance set to " + String(tank_bottom_distance));
      server->send(200, "text/html", "<H3>tank_bottom_distance set to " + String(tank_bottom_distance) + "</H3>");
      saveConfig();
    }
  });


  server->on("/showconfig", []() {
    server->send(200, "text/html", "speed_of_sound=" + String(speed_of_sound) + ",tank_bottom_distance=" + String(tank_bottom_distance) 
      + ",max_water_level = " + String(max_water_level) + ",distance_to_water = " + String(distance_to_water));
  });

  server->on("/getconfig", []() {
    server->send(200, "text/html", "speed_of_sound=" + String(speed_of_sound) + ",tank_bottom_distance=" + String(tank_bottom_distance) 
      + ",max_water_level=" + String(max_water_level) + ",distance_to_water=" + String(distance_to_water));
  });

  server->on("/d", []() {
    server->send(200, "text/html", "distance_to_water=" + String(distance_to_water));
  });


  
  server->onNotFound(handleNotFound);

  server->begin();
}

// Workaround to call readSpiffsConfig() before setup()
int rrr = readSpiffsConfig();





// Function that runs once when the ESP8266 initializes

void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  http.setReuse(true);
  //Serial.println("setup: Reading configuration from SPIFFS ...");
  //readSpiffsConfig();
  Serial.println("setup: Trying AutoConnect...");
  WiFiManager wifiManager;
  wifiManagerSetup(wifiManager);
  Serial.print("setup: Connected to Router: IP = ");
  Serial.println(WiFi.localIP());
  if (shouldSaveConfig) { 
    saveConfig();
  }
  createWebServer();  
  Serial.println("setup: HTTP server started. (Config Page at '/config', show conf at '/showconfig' or '/getconfig' )");

  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(100);
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(100);
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(100);
  digitalWrite(BUILTIN_LED, LOW);
  delay(1000);
}


// Function that repeatedly runs, doing the measuring and the sending

void loop() {
  digitalWrite(BUILTIN_LED, HIGH);
  digitalWrite(trigger, LOW);  
  delayMicroseconds(2); 
  
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10); 
  
  digitalWrite(trigger, LOW);
  duration = pulseIn(echo, HIGH, 60000);
  distance_to_water = (duration/2) * (String(speed_of_sound)).toFloat()/10000.0 ;

  long water_level = String(tank_bottom_distance).toInt() - distance_to_water;

  int normalized_level = ledcount * ((float)water_level / String(max_water_level).toFloat());
  if(normalized_level < 0) normalized_level = 0;
  if(normalized_level > ledcount) normalized_level = ledcount;

  Serial.println("distance_to_water = " + String(distance_to_water));
  Serial.println("water_level = " + String(water_level));
  Serial.println("normalized_level = " + String(normalized_level));

  if(abs(previous_level - water_level) < 8) {   // 8 cm waves peak-to-peak
    fakeCount = 0;
    previous_level = water_level;
    http.begin("http://192.168.4.1/set?n=" + String(normalized_level));
    int httpCode = http.GET();
    Serial.print("HTTP Response code: ");
    Serial.println(httpCode);
    http.end();
    delay(100);
    digitalWrite(BUILTIN_LED, LOW);
  } else {
    fakeCount++;

    http.begin("http://192.168.4.1/debug?m=" + String("fakeCount:") + String(fakeCount) + String(",water_level:") + String(water_level));
    int httpCode = http.GET();
    Serial.println(httpCode);
    http.end();
    Serial.print("fakeCount: ");
    Serial.println(fakeCount);
    
    if(fakeCount > 3) {
      fakeCount = 0;
      previous_level = water_level;
    }

    Serial.print("Ignoring fake water_level : ");
    Serial.println(water_level);
    digitalWrite(BUILTIN_LED, LOW);
    delay(100);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(100);
    digitalWrite(BUILTIN_LED, LOW);
    delay(100);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(100);
    digitalWrite(BUILTIN_LED, LOW);
    delay(100);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(100);
    digitalWrite(BUILTIN_LED, LOW);

    
  }
  delay(4000);
  
  server->handleClient();
}
