#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "time.h"

//Variable Wifi
const char* ssid       = "AndroidAP_4359";
const char* password   = "RSENSE-2020";

//Variables tiempos registrados
int numDato = 0;
unsigned long tiempoDato;

//Variables timers 
hw_timer_t * timer = NULL;
hw_timer_t * timeTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
boolean dato = false;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//variables auxiliares
boolean datoStart = false;
String estado;

String processor(const String& var){        
  Serial.println(var);
  if(var == "STATE"){
    return estado;
  } else if (var == "NUMDATO"){
    return String(numDato);
  } else if (var == "TIMEDATO"){
    return String(tiempoDato/1000000*2.5);  //paso de microsegundos a segundos
  }
  return String();
}

void IRAM_ATTR onTimer(){
  dato = true;                          //permite tomar valores del acelerometro
}

void IRAM_ATTR onMicroTimer(){
  tiempoDato++;                         //cuenta un microsegundo
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //Timer de frecuencia de datos
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);

  //Timer tiempo datos. 
  timeTimer = timerBegin(1, 8, true);             //cuenta 10e6 veces por segundo en el timer 1
  timerAttachInterrupt(timeTimer, &onMicroTimer, true);
  timerAlarmWrite(timeTimer, 25, true);               //interrupción cuando llegue a 10 o lo que es lo mismo, 1 micro s
  timerAlarmEnable(timeTimer);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  
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
    datoStart = false;
    //guarda el fichero en la sd
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
    server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request){
    tiempoDato = 0; 
    datoStart = true; 
    //crea fichero en la sd
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Start server
  server.begin();
}
 
void loop(){
    if (dato && datoStart){
      Serial.print("Tomo los valores: ");
      Serial.println(tiempoDato/1000000);
      //tomo los datos y los escribo en la sd separado por coma 
      dato = false;
    }
}
