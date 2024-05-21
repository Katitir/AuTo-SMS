#include <Wire.h>
#include <Arduino.h>
#include <max6675.h>
#include <HardwareSerial.h>
#include <EmonLib.h>

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;
static float tempC;
static float realPower;
static float powerFactor;
static float supplyVoltage;
static float Irms;
float Freq;
float Rpm;
char incomingByte;
String inputString;
String fuelLevel;
const int thermoDO = 19;
const int thermoCS = 5;
const int thermoCLK = 18;
const int relay = 26;
#define sw1 23
#define sw2 14

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
EnergyMonitor emon1;

void updateSerial();
void conditions();
void sendSMS();
void reply(String text);

void tempSensor(void *parameter){

  while(true){
   tempC = thermocouple.readCelsius();
   vTaskDelay(300/portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
} 

void powerSensor(void *parameter){

  while (true){

    emon1.calcVI(64,1000);
    realPower = emon1.realPower;
    powerFactor = emon1.powerFactor;
    supplyVoltage = emon1.Vrms;
    Irms = emon1.Irms;

    vTaskDelay(600/portTICK_PERIOD_MS);
  }
   vTaskDelete(NULL);
 }

void rpm(void *parameter){

  while(true){
  Freq = (supplyVoltage/220)*60;
  Rpm = (Freq*120)/4;

  vTaskDelay(700/portTICK_PERIOD_MS); }
  vTaskDelete(NULL);
}

void setup() {
  
  Serial.begin(115200);
  Serial2.begin(115200);
  emon1.voltage(32, 414, 1.7);     // Voltage: input pin, calibration, phase_shift
  emon1.current(33, 9.7);         // Current: input pin, calibration.
  digitalWrite(relay, HIGH);
  delay(3000);
  pinMode(relay, OUTPUT);
  pinMode(sw1, INPUT_PULLUP);
  pinMode(sw2, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial2.println("AT+CMGF=1");   //Configure in text mode
  delay(1000);
  Serial2.println("AT+CNMI=1,2,0,0,0");  //procedure, how to recive messages from the net
  delay(1000);
  Serial2.println("AT+CMGL=\"REC UNREAD\""); //Read unredad messages
  
    xTaskCreatePinnedToCore(
          rpm,
          "rpm",
          1024,
          NULL,
          1,
          NULL,
          app_cpu );
    
    xTaskCreatePinnedToCore(
          tempSensor,
          "tempSensor",
          1024,
          NULL,
          1,
          NULL,
          app_cpu ); 

    xTaskCreatePinnedToCore(
          powerSensor,
          "Power Sensor",
          3024,
          NULL,
          1,
          NULL,
          app_cpu );

  delay(12000);
}

void loop() {

  conditions();
  updateSerial();

 
  static unsigned long lastMessageTime = 0;
  unsigned long currentTime = millis();

    if (currentTime - lastMessageTime >= 8000) { //Sends every 1 hour but can be changed anytime you want hour
      sendSMS();
      lastMessageTime = currentTime;
    }

  updateSerial();
 
}

void updateSerial(){

    if(Serial2.available()){

        Serial.write(Serial2.read());

        while (Serial2.available())
        {
          incomingByte = Serial2.read();
          inputString += incomingByte;
        }
        delay(10);
       // Serial.println(inputString);
        inputString.toUpperCase();

        if(inputString.indexOf("OFF") > -1){
          digitalWrite(relay, LOW);
          digitalWrite(LED_BUILTIN, HIGH);
          delay(10000);
        }
         inputString == "";
      }

      if(Serial.available()){
        Serial2.write(Serial.read());
    }
}

void conditions(){                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      

bool s1 = digitalRead(sw1);
bool s2 = digitalRead(sw2);
//Low Load
  if(tempC > 80){
    reply("Warning High Temperature Detected. System shutting Down");
    sendSMS();
    delay(2000);
    digitalWrite(relay, LOW);
    delay(5000); }

  else  if(supplyVoltage > 240){
      reply("Generator is Over Voltage. System shutting Down");
      sendSMS();
      delay(2000);
      digitalWrite(relay, LOW); 
       
      delay(5000);  }
 /*
  else  if(supplyVoltage < 215){
      reply("Generator is Under Voltage. System shutting Down");
      sendSMS();
      digitalWrite(relay, LOW);
      delay(5000);  }
*/
  else  if(Irms > 2){
      sendSMS();
      delay(2000);
      digitalWrite(relay, LOW);
      reply("Generator is Overcurrent");
      delay(5000);
       } else { digitalWrite(relay, HIGH);}

  if (s1 == LOW && s2 == HIGH) {
     fuelLevel = "Fuel is Low";}
  else if (s1 == HIGH && s2 == LOW) {
     fuelLevel = "Fuel is High"; }
  else if (s1 == HIGH && s2 == HIGH) {
     fuelLevel = "Fuel is Medium";
   } 
}

void sendSMS()  {

  updateSerial();
  Serial2.println("AT+CMGS=\"+639310553652\"\r");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  Serial2.print("Temperature:" ); //text content
  Serial2.println(tempC ); //text content
  Serial2.print("Voltage:");
  Serial2.println(supplyVoltage);
  Serial2.print("Current:");
  Serial2.println(Irms);
  Serial2.print("Power:");
  Serial2.println(realPower);
  Serial2.print("Power Factor:");
  Serial2.println(powerFactor);
  Serial2.print("Frequency:");
  Serial2.println(Freq);
  Serial2.print("RPM:");
  Serial2.println(Rpm);
  Serial2.println(fuelLevel);
  updateSerial();
  delay(1000);
  Serial.println("Message Sent");
  Serial2.write(26); 
  delay(600);
}

void reply(String text){
  updateSerial();
  Serial2.println("AT+CMGS=\"+639310553652\"\r");
  updateSerial();
  Serial2.println(text);
  updateSerial();
  delay(100);
  Serial2.write(26);
  delay(600);
}



 //Debugging Purposes
 /*
  Serial.print(tempC);
  Serial.print(",");
  Serial.print(supplyVoltage);
  Serial.print(",");
  Serial.print(Irms);
  Serial.print(",");
  Serial.print(realPower);
  Serial.print(",");
  Serial.print(powerFactor);
  Serial.print(",");
  Serial.print(Freq);
  Serial.print(",");
  Serial.print(Rpm);
  Serial.println(",");
  delay(5000);
*/