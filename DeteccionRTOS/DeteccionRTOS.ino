#include <Arduino.h>
#include <Wire.h>

int pinSCL = 5;
int pinSDA = 17;
#define    MPU9250_ADDRESS            0x68    //Direccion del MPU
#define    GYRO_FULL_SCALE_2000_DPS   0x18    //Escala del giroscopo de 2000º/s
#define    ACC_FULL_SCALE_16_G        0x18    //Escala del acelerometro de +/-16g
#define    A_R         ((32768.0/2.0)/9.8)    //Ratio de conversion

int16_t aX, aY, aZ, gX, gY, gZ;

int periodoSensar = 1000; 
int prioridadSensar = 1; 

int periodoPreproceso = 200;
int prioridadPreproceso = 2;

int periodoClasificacion = 200;
int prioridadClasificacion = 2;

int ventanaMedidas = 1000; //1000 por empezar con algo
int ventanaTotal = ventanaMedidas * 2;
int medidas[2000][6]; //Array de x medidas * 6 lecturas (3 de acelerometro + 3 de giroscopo)
int pasoTemporal = 1; //tiempo que transcurre entre una medida y la siguiente

boolean primeraMedida,segundaMedida = false;
boolean terceraMedida = true;

int IaX,IaY,IaZ,iaX,iaY,iaZ;
float mediaX,mediaY,mediaZ;

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


void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(pinSDA, pinSCL);
  I2CwriteByte(MPU9250_ADDRESS, 28, ACC_FULL_SCALE_16_G);
  I2CwriteByte(MPU9250_ADDRESS, 27, GYRO_FULL_SCALE_2000_DPS);
  
  xTaskCreate(sensar,"task1",1000,NULL,prioridadSensar,NULL);   //Nombre de la función, Nombre descriptivo para la tarea 1, este valor indica el numero de palabras que el stack puede soportar, Parametros de entrada de la función, Prioridad de la función (mayor cuanto mayor sea el número), __ 
  xTaskCreate(preproceso,"task2",1000,NULL,prioridadPreproceso,NULL);
  xTaskCreate(clasificacion,"task2",1000,NULL,prioridadClasificacion,NULL);  
  //vTaskStartScheduler();
}

void loop() {

}

void sensar(void *pvParameters){         //declaro la tarea 1
  int i = 0;
  while (1){
    i++;
    uint8_t aceleracion[6];                               //Creo una cadena de 6 bytes para almacenar las distintas lecturas
      I2Cread(MPU9250_ADDRESS, 0x3B, 6, aceleracion);
      medidas[i][1] = (aceleracion[0] << 8 | aceleracion[1])/A_R;  //aX
      medidas[i][2] = (aceleracion[2] << 8 | aceleracion[3])/A_R;  //aY
      medidas[i][3] = (aceleracion[4] << 8 | aceleracion[5])/A_R;  //aZ
    uint8_t giroscopo[6];
      I2Cread(MPU9250_ADDRESS, 0x43, 6, giroscopo);
      medidas[i][4] =  (giroscopo[0] << 8 | giroscopo[1]);         //gX
      medidas[i][5] =  (giroscopo[2] << 8 | giroscopo[3]);         //gY
      medidas[i][6] =  (giroscopo[4] << 8 | giroscopo[5]);         //gZ
    if (i == 2 * ventanaMedidas){
      i = 0;
    }
    vTaskDelay (periodoSensar);
  }
}
  
void preproceso(void *pvParameters){         //declaro la tarea 2
  int valorInicial, valorFinal;
  
  if ((medidas[ventanaMedidas][6] =! '\0') && (terceraMedida)) {             //primeraVentana 0-50%
    valorInicial = 0;
    valorFinal = ventanaMedidas;
    terceraMedida = false;
    primeraMedida = true;
    
  } else if ((medidas[3*2*ventanaMedidas/4][6] =! '\0')  && (primeraMedida)) {  //segungaVentana 25-75%
    valorInicial = ventanaMedidas/4;
    valorFinal = 3*ventanaMedidas/4;
    primeraMedida = false;
    segundaMedida = true;    
    
  } else if ((medidas[2*ventanaMedidas][6] =! '\0')  && (segundaMedida))  {  //segungaVentana 50-100%
    valorInicial = ventanaMedidas;
    valorFinal = 2 * ventanaMedidas;  
    segundaMedida = false;
    terceraMedida = true;  
  
  }

    //Calculo de la derivada
    for  (int aux = valorInicial ; aux == valorFinal ; aux++) {
//      daX = sqrt( (medidas[aux+1][1] - medidas[aux][1])^2 + (pasoTemporal)^2 );
//      daY = sqrt( (medidas[aux+1][2] - medidas[aux][2])^2 + (pasoTemporal)^2 );
//      daZ = sqrt( (medidas[aux+1][3] - medidas[aux][3])^2 + (pasoTemporal)^2 );

      iaX =+ medidas[aux][1]; 
      iaY =+ medidas[aux][2];
      iaZ =+ medidas[aux][3];
      
//      dgX = sqrt( (medidas[aux+1][4] - medidas[aux][4])^2 + (pasoTemporal)^2 );
//      dgY = sqrt( (medidas[aux+1][5] - medidas[aux][5])^2 + (pasoTemporal)^2 );
//      dgZ = sqrt( (medidas[aux+1][6] - medidas[aux][6])^2 + (pasoTemporal)^2 );

      for (int cnt = 1; cnt ==6; aux++){                                   //Una vez he leido el valor 
        medidas[aux][cnt] = '\0';
      }
    }

    IaX = iaX;
    IaY = iaY;
    IaZ = iaZ;

    mediaX = IaX/1000;
    mediaY = IaZ/1000;
    mediaZ = IaZ/1000; 

    iaX = 0;
    iaY = 0;
    iaZ = 0;
   
    vTaskDelay (periodoPreproceso);
  
}

void clasificacion(void *pvParameters){         //declaro la tarea 2
  String movimiento;
  while(1) {
    if ( (mediaX < 46) && (mediaX > 23) ) {
      movimiento = "globo";
    }

    if ( (mediaZ < 11) && (mediaZ > 6) ) {
      movimiento = "pase";
    }

    if ( (mediaZ < 32) && (mediaZ > 15) ) {
      if ( (mediaY < 27) && (mediaY > 15) ) {
          movimiento = "placaje";
      }
    }

    }
    vTaskDelay (periodoClasificacion);
}
