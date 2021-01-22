#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "time.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <Wire.h>

#include <ESP32_FTPClient.h>

//Variable Wifi
const char* ssid       = "AndroidAP_4359";
const char* password   = "RSENSE-2020";

//Variables FTP
char ftp_server[] = "155.210.150.77";
char ftp_user[]   = "rsense";
char ftp_pass[]   = "rsense";
ESP32_FTPClient ftp (ftp_server,ftp_user,ftp_pass, 5000, 2);

//Variables tiempos registrados
int numDato = 0;
unsigned long tiempoDato;

//Variables timers 
hw_timer_t * timer = NULL;
hw_timer_t * timeTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
boolean dato = false;

//Variables acelerometro
int pinSCL = 5;
int pinSDA = 17;
#define    MPU9250_ADDRESS            0x68    //Direccion del MPU
#define    GYRO_FULL_SCALE_2000_DPS   0x18    //Escala del giroscopo de 2000º/s
#define    ACC_FULL_SCALE_2_G         0x00    //Escala del acelerometro de +/-2g
#define    A_R         ((32768.0/2.0)/9.8)    //Ratio de conversion
int16_t aX, aY, aZ, gX, gY, gZ= 0.0;

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
String estado, nombreString, infoString;  //movimiento que registra
char cabecera[60] = "accX,accY,accZ,gyroX,gyroY,gyroZ,Movimiento,Tiempo\n";
int numRegistros = 0;  
char nombreChar[25];  //Nombre del archivo a guardar en la SD
char infoChar[100];

String processor(const String& var){        
  Serial.println(var);
  if(var == "STATE"){
    return estado;
  } else if (var == "NUMDATO"){
    return String(numDato);
  } else if (var == "TIMEDATO"){
    return String(tiempoDato/1000000 );  //paso de microsegundos a segundos
  }
  return String();
}

void IRAM_ATTR onTimer(){
  dato = true;                          //permite tomar valores del acelerometro
}

void IRAM_ATTR onMicroTimer(){
  tiempoDato++;                         //cuenta un microsegundo
}

void rutaStart(){
    tiempoDato = 0;  
    numRegistros++;
    getLocalTime(&timeinfo);
    timeMes = timeinfo.tm_mon;
    timeDia = timeinfo.tm_mday;
    timeHora = timeinfo.tm_hour;
    //nombreString = String(timeMes) + "-" + String(timeDia) +  "-" + String(timeHora) +  "-" + String(numRegistros) + ".csv";
    nombreString = String(timeMes) + String(timeDia) + String(timeHora) + String(numRegistros) + ".csv";
    nombreString.toCharArray(nombreChar, 20);
    Serial.println(nombreChar);
    ftp.InitFile("Type A");     //Creo un archivo tipo A(por defecto)
    ftp.NewFile(nombreChar);   //Nombre del archivo y tipo
    datoStart = true;
}

void I2Cread(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data){
   Wire.beginTransmission(Address);       //Indica la dirección del esclavo al que me quiero dirigir
   Wire.write(Register);                  //Indico el registro con el que me quiero comunicar
   Wire.endTransmission();
 
   Wire.requestFrom(Address, Nbytes);     //Leo del esclavo que he seleccionado Nbytes
   uint8_t index = 0;
   while (Wire.available())
      Data[index++] = Wire.read();        //Almaceno la lectura en un vector (Data).
}

void I2CwriteByte(uint8_t Address, uint8_t Register, uint8_t Data){
   Wire.beginTransmission(Address);       //Inicio la comunicación con el esclavo que quiero
   Wire.write(Register);                  //Accedo al registro en el que quiero escribir
   Wire.write(Data);                      //Escribo los datos pertinentes. (Normalmente la configuración del registro)
   Wire.endTransmission();
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
  timer = timerBegin(0, 8, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 100, true);
  timerAlarmEnable(timer);

  //Timer tiempo datos. 
  timeTimer = timerBegin(1, 8, true);             //cuenta 10e6 veces por segundo en el timer 1
  timerAttachInterrupt(timeTimer, &onMicroTimer, true);
  timerAlarmWrite(timeTimer, 50, true);               //interrupción cuando llegue a 10 o lo que es lo mismo, 2 micro s
  timerAlarmEnable(timeTimer);

  //Set up acelerometro
  Wire.begin(pinSDA, pinSCL);
  I2CwriteByte(MPU9250_ADDRESS, 28, ACC_FULL_SCALE_2_G);
  I2CwriteByte(MPU9250_ADDRESS, 27, GYRO_FULL_SCALE_2000_DPS);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  ftp.OpenConnection();
  ftp.ChangeWorkDir("rsense/jmonzon");  //Me muevo al directorio
  
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
    ftp.CloseFile();
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
    uint8_t aceleracion[6];                               //Creo una cadena de 6 bytes para almacenar las distintas lecturas
      I2Cread(MPU9250_ADDRESS, 0x3B, 6, aceleracion);
      aX = (aceleracion[0] << 8 | aceleracion[1])/A_R; 
      aY = (aceleracion[2] << 8 | aceleracion[3])/A_R; 
      aZ = (aceleracion[4] << 8 | aceleracion[5])/A_R;  
    uint8_t giroscopo[6];
      I2Cread(MPU9250_ADDRESS, 0x43, 6, giroscopo);
      gX =  (giroscopo[0] << 8 | giroscopo[1]); 
      gY =  (giroscopo[2] << 8 | giroscopo[3]); 
      gZ =  (giroscopo[4] << 8 | giroscopo[5]); 
      //tomo los datos y los escribo en la sd separado por coma 
      infoString = String(String(aX) + "," + String(aY) +  "," + String(aZ) +  "," + String(gX) + "," + String(gY) +  "," + String(gZ) + "," + estado + "," + String(tiempoDato) + "\n" );
      infoString.toCharArray(infoChar, 100);
      ftp.Write(infoChar);
      dato = false;
    }
}
