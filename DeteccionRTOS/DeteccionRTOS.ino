#include <Arduino.h>
#include <Wire.h>
#include <analogWrite.h>

int pinSCL = 5;
int pinSDA = 17;
#define    MPU9250_ADDRESS            0x68    //Direccion del MPU
#define    ACC_FULL_SCALE_2_G         0x00    //Escala del acelerometro de +/-2g
#define    A_R         ((32768.0/2.0)/9.8)    //Ratio de conversion

int periodoSensar = 1; 
int prioridadSensar = 3; 

int periodoVentana = 150;
int prioridadVentana = 2;

int periodoClasificacion = 150;
int prioridadClasificacion = 1;

int ventanaMedidas = 400; //1000 por empezar con algo
int ventanaTotal = ventanaMedidas * 2;
int medidas[800]; //Como solo me interesa la velocidad en x solo necesito un array
int pasoTemporal = 1; //tiempo que transcurre entre una medida y la siguiente
int valorInicial, valorFinal;

boolean primeraMedida,segundaMedida,terceraMedida = false;
boolean cuartaMedida = true;

int datos,procesado = 0;

float iaX[20], IaX;
int integral;
float diff, media, Media;
double desv, desv1, desviacion; 

void I2Cread(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data)
{
   Wire.beginTransmission(Address);       //Indica la dirección del esclavo al que me quiero dirigir
   Wire.write(Register);                  //Indico el registro con el que me quiero comunicar
   Wire.endTransmission();
 
   Wire.requestFrom(Address, Nbytes);     //Leo del esclavo que he seleccionado Nbytes
   uint8_t index = 0;
   while (Wire.available())
      Data[index++] = Wire.read();        //Almaceno la lectura en un vector (Data).
}

void I2CwriteByte(uint8_t Address, uint8_t Register, uint8_t Data)
{
   Wire.beginTransmission(Address);       //Inicio la comunicación con el esclavo que quiero
   Wire.write(Register);                  //Accedo al registro en el que quiero escribir
   Wire.write(Data);                      //Escribo los datos pertinentes. (Normalmente la configuración del registro)
   Wire.endTransmission();
}

void Color(int R, int G, int B){     
   analogWrite(23, R) ;   // Red 
   analogWrite(22, G) ;   // Green 
   analogWrite(21, B) ;   // Blue 
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(23, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(21, OUTPUT);

  Wire.begin(pinSDA, pinSCL);
  I2CwriteByte(MPU9250_ADDRESS, 28, ACC_FULL_SCALE_2_G);
  
  xTaskCreate(sensar,"AAAAAAAA",1000,NULL,prioridadSensar,NULL);  
  xTaskCreate(ventana,"BBBBBBB",100000,NULL,prioridadVentana,NULL);
//  xTaskCreate(preproceso,"CCCCCCCC",10000,NULL,prioridadPreproceso,NULL);
//  xTaskCreate(calculoEstadistico,"DDDDDDD",1000,NULL,prioridadCalculo,NULL);
  xTaskCreate(clasificacion,"EEEEEEEE",1000,NULL,prioridadClasificacion,NULL);  
  //vTaskStartScheduler();
}

void loop() {

}

void sensar(void *pvParameters){         //declaro la tarea 1
  int i = 0;
  while (1){
    i++;
    
    uint8_t aceleracion[2];                               //Creo una cadena de 6 bytes para almacenar las distintas lecturas
      I2Cread(MPU9250_ADDRESS, 0x3B, 6, aceleracion);
      medidas[i] = (aceleracion[0] << 8 | aceleracion[1])/A_R;  //aX
      //Serial.println(medidas[i]);
      
    if (i == 2 * ventanaMedidas){
      i = 0;
    }
    //Serial.println("Tomo valores");
    datos++;
    vTaskDelay (periodoSensar);
  }
}

void ventana(void *pvParameters){         //declaro la tarea 1
  int i = 0;
  int aux;
  while (1){
    if ((medidas[400] =! '\0') && (cuartaMedida)) {           //primeraVentana 0-50%
      valorInicial = 0;
      valorFinal = ventanaMedidas;
      cuartaMedida = false;
      primeraMedida = true;
    
    } else if ((medidas[600] =! '\0')  && (primeraMedida)) {  //segundaVentana 25-75%
      valorInicial = ventanaMedidas/2;
      valorFinal = 3*ventanaMedidas/2;
      primeraMedida = false;
      segundaMedida = true;    
    
    } else if ((medidas[800] =! '\0')  && (segundaMedida))  {  //terceraVentana 50-100%
      valorInicial = ventanaMedidas;
      valorFinal = 2*ventanaMedidas;  
      segundaMedida = false;
      terceraMedida = true;  
  
    } else if ((medidas[200] =! '\0')  && (terceraMedida))  {  //cuartaVentana 75-25%
      valorInicial = 3*ventanaMedidas/2;
      valorFinal = 200;  
      terceraMedida = false;
      cuartaMedida = true;  
    }

    for  (int i = 0 ; i < 400 ; i++) {      //Leo todos los datos disponibles la ventana de 400 datos
      aux = valorInicial + i;                      //Para no tener problemas de overflow creo una variable auxiliar con la que accedere a los datos
      if (aux >= 800){                             //Si intento leer la posición 800 (no existe) leere la pos 0, evitando asi el overflow
        aux = aux - 800;
      }
    IaX = IaX + medidas[aux]; 
    }
    integral = IaX;
    Serial.print("P:");
    Serial.println(integral);
    Serial.println("----------");
    Media = integral/400;        
    IaX = 0;

    for (int i = 0; i < 400; i++){
      int aux2;
      aux2 = valorInicial + i;                      
      if (aux2 >= 800){                             
        aux2 = aux2 - 800;
      }
      diff = (medidas[aux2]) - Media;
      desv1 = desv1 + pow(diff,2.0); 
    }
    
    desv = sqrt(desv1/(20 - 1)); //calculo de la desviacion
    desviacion = desv;
    desv1 = 0;
    
    vTaskDelay (periodoVentana);
  }
}

void clasificacion(void *pvParameters){         //declaro la tarea 2
  //String mov = "";
  while(1) {
    if (desv < 7.5){
      if (integral <5200){
        //mov = "Balon parado horizontal";
        Color(0,255,0);   //Verde
      } else{
        //mov = "Balon parado vertical";
        Color(0,0,255);    //Azul
      }
    } else {
      if (integral <5000){
        //mov = "Pase";
        Color(255,117,020);  //Naranja
      } else if (integral <6800){
        //mov = "Globo";
        Color(255,0,0);     //Rojo
      } else {
        //mov = "Placaje";
        Color(163, 73, 164); //Morado
      }
    }
    Serial.print("C:");
    Serial.println(desv);
    //Serial.println(integral);
    //Serial.println("--------");    
    //Serial.println("Tarea de clasificacion");
    vTaskDelay (periodoClasificacion);
  }
}
