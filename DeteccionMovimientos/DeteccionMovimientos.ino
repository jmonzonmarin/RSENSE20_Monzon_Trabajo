#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "time.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

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

//NTP Server:
const char* ntpServer = "europe.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
tm timeinfo;
int timeMes, timeDia, timeHora;

//variables auxiliares
boolean datoStart = false;
String estado;  //movimiento que registra
char cabecera[60] = "accX,accY,accZ,gyroX,gyroY,gyroZ,Movimiento,Tiempo\n";
int numRegistros = 0;
String nombreString;  
char nombreChar[25];  //Nombre del archivo a guardar en la SD

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

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void rutaStart(){
    tiempoDato = 0;  
    numRegistros++;
    getLocalTime(&timeinfo);
    timeMes = timeinfo.tm_mon;
    timeDia = timeinfo.tm_mday;
    timeHora = timeinfo.tm_hour;
    nombreString = String("/" + String(timeMes) + "-" + String(timeDia) +  "-" + String(timeHora) +  "-" + String(numRegistros) + ".csv");
    nombreString.toCharArray(nombreChar, 20);
    //crea fichero en la sd
    writeFile(SD, nombreChar, cabecera);
    datoStart = true;
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

  //Init SD
  if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return;
    }
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  //Set up NTP 
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
    server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request){
    rutaStart();
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
      appendFile(SD, nombreChar, "World!\n");
      dato = false;
    }
}
