//#include <Arduino_JSON.h>
#include <ArduinoJson.h>

#include <PZEM004Tv30.h>
#include <HardwareSerial.h>

#include "EmonLib.h"             // Include Emon Library
EnergyMonitor emon1;             // Create an instance
#include <AWS_IOT.h> // para aws
#include <WiFi.h>   // para aws

/*
 * //https://github.com/Savjee/EmonLib-esp32
 * https://github.com/mandulaj/PZEM-004T-v30
 * https://randomnerdtutorials.com/esp32-ssd1306-oled-display-arduino-ide/
*/
HardwareSerial PzemSerial2(2);     // Use hwserial UART2 at pins IO-16 (RX2) and IO-17 (TX2)
PZEM004Tv30 pzem(&PzemSerial2);

//variables led

StaticJsonBuffer<300> JSONBuffer;

//DynamicJsonBuffer jsonBuffer;

int red = 19;
int green = 5;
int blue = 18;

// variables AWS

AWS_IOT hornbill;

char WIFI_SSID[]="Maferu";
char WIFI_PASSWORD[]="66983717";
char HOST_ADDRESS[]="ayedcd8eu2d2s-ats.iot.us-east-1.amazonaws.com";
char CLIENT_ID[]= "MyIoTPolicie";
char TOPIC_NAMESUB[]= "powermonitoring-app/buildind/esp32/pub";
char TOPIC_NAMEPUB[]= "powermonitoring-app/buildind/esp32/sub";


int status = WL_IDLE_STATUS;
int tick=0,msgCount=0,msgReceived = 0;
char payload[512];
char rcvdPayload[512];


void setup() {
emon1.voltage(A0, 598.760, 0);  // Voltage: input pin, calibration, phase_shift
emon1.current(A6, 0.40513938);       // Current: input pin, calibration.
  
  Serial.begin(115200);
  delay(2000);

//  RGB

// initialize the digital pin as an output.
pinMode(red, OUTPUT);
pinMode(green, OUTPUT);
pinMode(blue, OUTPUT);
digitalWrite(red, LOW);
digitalWrite(green, HIGH);
digitalWrite(blue, LOW);

// acceso a aws

 while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(WIFI_SSID);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // wait 5 seconds for connection:
        delay(5000);
    }

    Serial.println("Connected to wifi");

    if(hornbill.connect(HOST_ADDRESS,CLIENT_ID)== 0)
    {
        Serial.println("Connected to AWS");
        delay(1000);

        if(0==hornbill.subscribe(TOPIC_NAMESUB,mySubCallBackHandler))
        {
            Serial.println("Subscribe Successfull");
        }
        else
        {
            Serial.println("Subscribe Failed, Check the Thing Name and Certificates");
            while(1);
        }
    }
    else
    {
        Serial.println("AWS connection failed, Check the HOST Address");
        while(1);
    }
// fin conexión AWS


}

void loop() {

    float voltage = pzem.voltage();
    float current = pzem.current();
    float power = pzem.power();
    float energy = pzem.energy();
    float frequency = pzem.frequency();
    float pf = pzem.pf();


    float powerFActor     = emon1.powerFactor;      //extract Power Factor into Variable
    float supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
    float Irms            = emon1.Irms;             //extract Irms into Variable
    float realPower       = supplyVoltage*Irms; //emon1.realPower;        //extract Real Power into variable

    emon1.calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
    Serial.println("Circuit1: Emonlib ");
    //emon1.serialprint();           // Print out all variables (realpower, apparent power, Vrms, Irms, power factor)
    printParemeters( supplyVoltage, Irms, realPower, 0, powerFActor ); 
    Serial.println("Circuit2 PZEM: ");
    printParemeters( voltage, current, power, energy, pf ); 


    Serial.println();
    delay(1000);

// publish

// si recibe mensages

//  cambio led

     if(msgReceived == 1)
    {
      JsonObject& root = JSONBuffer.parseObject(rcvdPayload);
      String LED = root["message"];
        msgReceived = 0;
        Serial.print("Received Message:");
        Serial.println(rcvdPayload);
        
  
        if (LED.equals("R")){
          digitalWrite(red, HIGH);
          digitalWrite(green, LOW);
          digitalWrite(blue, LOW);
        }

        if (LED.equals("G")){
          digitalWrite(red, LOW);
          digitalWrite(green, HIGH);
          digitalWrite(blue, LOW);
        }
    }


// para publicar mensages

    if(tick >= 5)   // publish to topic every 5seconds
    {
        tick=0;

          if(voltage < 100 || supplyVoltage < 100){
              digitalWrite(red, HIGH);
              digitalWrite(green, LOW);
              digitalWrite(blue, LOW);
           }
        
        sprintf(payload,"{\"voltage1\" : \"%f\", \"current1\" : \"%f\", \"power1\" : \"%f\", \"voltage2\" : \"%f\", \"current2\" : \"%f\", \"power2\" : \"%f\"}",
        voltage, current, power, supplyVoltage, Irms, realPower);
        
        if(hornbill.publish(TOPIC_NAMEPUB,payload) == 0)
        {        
            Serial.print("Publish Message:");
            Serial.println(payload);
        }
        else
        {
            Serial.println("Publish failed");
        }
    }  
    vTaskDelay(1000 / portTICK_RATE_MS); 
    tick++;
}


//metodo AWS para mandejar los mensajes entrantes
void mySubCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
    strncpy(rcvdPayload,payLoad,payloadLen);
    rcvdPayload[payloadLen] = 0;
    msgReceived = 1;
}

void printParemeters(float voltage,float current,float power,float energy,float pf ){
  if( !isnan(voltage) ){
        Serial.print(voltage); Serial.print("V ");
    } else {
        Serial.print("--V ");
    }

     if( !isnan(current) ){
       Serial.print(current); Serial.print("A ");
    } else {
        Serial.print("-- ");
    }

    
    if( !isnan(power) ){
        Serial.print(power); Serial.print("W ");
    } else {
        Serial.print("--W ");
    }

    if( !isnan(energy) ){
        Serial.print(energy,3); Serial.print("kWh ");
    } else {
        Serial.print("--kWh ");
    }

  if( !isnan(pf) ){
        Serial.print("PF: "); Serial.println(pf);
    } else {
        Serial.println("--");
    }}
    
