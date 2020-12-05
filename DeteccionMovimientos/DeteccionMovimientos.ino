#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "time.h"

const char* ssid       = "AndroidAP_4359";
const char* password   = "RSENSE-2020";

const int ledPin = 2;
String ledState;
boolean muestraHora = false;

//NTP Server:
const char* ntpServer = "europe.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
tm timeinfo;
String fecha;
String estado;
int timeHour, timeMin, timeSec; 

int numDato = 0;
unsigned long tiempoStart; 
unsigned long tiempoStop;
unsigned long timeDato;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    return estado;
  } else if (var == "NUMDATO"){
    return String(numDato);
  } else if (var == "TIMEDATO"){
    Serial.println(timeDato);
    return String(timeDato);
  }
  return String();
}

void printLocalTime(){
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}
 
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to set GPIO to HIGH
  server.on("/run", HTTP_GET, [](AsyncWebServerRequest *request){
    estado = "Corriendo"; 
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW
  server.on("/tackle", HTTP_GET, [](AsyncWebServerRequest *request){
    estado = "Placaje";    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    estado = "Balon parado";    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/pass", HTTP_GET, [](AsyncWebServerRequest *request){
    estado = "Pase";    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/lineout", HTTP_GET, [](AsyncWebServerRequest *request){
    estado = "Touch";    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

    server.on("/parada", HTTP_GET, [](AsyncWebServerRequest *request){
    tiempoStop = 10; 
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
    server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request){
    tiempoStart = 5;  //guardar momento actual
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Start server
  server.begin();
}
 
void loop(){
    delay(1000);
    Serial.println(tiempoStart);
    Serial.println(tiempoStop);
}
