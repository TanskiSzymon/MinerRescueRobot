/*PLYTKA 
[env:lolin32_lite]
platform = espressif32
board = lolin32_lite
framework = arduino
monitor_speed = 74880
lib_deps = 
	knolleary/PubSubClient@^2.8
*/

#include <WiFi.h>
#include <string>
#include <PubSubClient.h>
#include <string>
#include <iostream>
#include <algorithm>

#define ledPin 2
#define TRIG_PIN 12
#define ECHO_PIN 14

#define LeftMotorPWM 13
#define LeftMotorIN1 17 
#define LeftMotorIN2 16
#define RightMotorPWM 33
#define RightMotorIN1 32 
#define RightMotorIN2 19

clock_t start ;
clock_t temp ;
clock_t czas;
long lastMsg = 0;
long lastMsgD = 0;
using namespace std;

//Local wifi ssid and password
const char* ssid ="YOUR WIFI NAME";   
const char* password = "YOUR WIFI PASSWORD";
//Your MQTT server
const char* mqtt_server ="broker.mqttdashboard.com";//"krab.pwr.edu.pl";
const char* mqtt_username = "ESPCTR0004";
const char* mqtt_password = "12345678";
const char* inTopic = "PUM2023v2/BETA/CTR";
const int mqtt_port =1883;
const int STOPdistance = 25; 
WiFiClient espClient;
PubSubClient client(espClient);

void stop(){
  digitalWrite(RightMotorIN1, HIGH);
  digitalWrite(RightMotorIN2, HIGH);
  analogWrite(RightMotorPWM, 0);
  
  digitalWrite(LeftMotorIN1, HIGH);
  digitalWrite(LeftMotorIN2, HIGH);
  analogWrite(LeftMotorPWM, 0);
}

void setRightMotor(int RightPWM, int RightDirection){
  // RightDirection 0 - forward, 1 - backward
  if (RightPWM<=100)
  {
    stop();
  }
  if (RightPWM>100){
    if ((RightPWM > 255) || (RightPWM <0))
      stop();
    if (RightDirection == 0){
      analogWrite(RightMotorPWM, RightPWM);
      digitalWrite(RightMotorIN1, HIGH); 
      digitalWrite(RightMotorIN2, LOW); 
    }
    if (RightDirection == 1){
     analogWrite(RightMotorPWM, RightPWM);
     digitalWrite(RightMotorIN2, HIGH); 
     digitalWrite(RightMotorIN1, LOW); 
    }
  }
}
void setLeftMotor(int LeftPWM, int LeftDirection){
  // RightDirection 0 - forward, 1 - backward
  if (LeftPWM<=100)
    stop();
  if (LeftPWM>100){
    if ((LeftPWM > 255) || (LeftPWM <0))
      stop();
    if (LeftDirection == 0){
      analogWrite(LeftMotorPWM, LeftPWM);
      digitalWrite(LeftMotorIN1, HIGH); 
      digitalWrite(LeftMotorIN2, LOW); 
    }
    if (LeftDirection == 1){
     analogWrite(LeftMotorPWM, LeftPWM);
     digitalWrite(LeftMotorIN2, HIGH); 
     digitalWrite(LeftMotorIN1, LOW); 
    }
  }
}

int checkDistanceAndStop() {
long now = millis();
  if (now - lastMsgD > 200) {
    lastMsgD = now;

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    long distance = (duration/2) / 29.1;
    Serial.print("\nDystans: ");
    Serial.print(distance);
    string distanceStr = to_string(distance);
    client.publish("PUM2023v2/BETA/DISTANCE", distanceStr.c_str()); 
    if(distance < STOPdistance) {
      stop();
      client.publish("PUM2023v2/BETA/ANSWER", "TO CLOSE"); 
      return 1;
    }
   else return 0;
  }
  return 0;
}
void watch_dog(){
  long now = millis();
  if (now - lastMsg > 145) {
    lastMsg = now;
    temp = clock();
    czas = (double)(temp - start) / CLOCKS_PER_SEC;
    if(czas>0.1){
        stop();
        client.publish("PUM2023v2/BETA/ANSWER", "Watch_dog stop!"); 
    }
  }
} 

void blink_led(unsigned int times, unsigned int duration){
  for (int i = 0; i < times; i++) {
    digitalWrite(ledPin, HIGH);
    delay(duration);
    digitalWrite(ledPin, LOW);  
    delay(200);
  }
}

void setup_wifi() {
  delay(50);
  WiFi.begin(ssid, password);
  int c=0;
  while (WiFi.status() != WL_CONNECTED) {
    blink_led(2,200); 
    c=c+1;
    if(c>10){
        ESP.restart(); //restart ESP after 10 seconds
    }
  }
}

void connect_mqttServer() {
  while (!client.connected()) {

    if(WiFi.status() != WL_CONNECTED){
      setup_wifi();
      Serial.print("\nNOT CONNECT to wifi! ");
    }
    String clientId = "ESPCTR0004";   // Create a random client ID
    clientId += String(random(0xffff), HEX);
    delay(100);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      client.subscribe(inTopic);   // subscribe the topics here
    } 
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  //Serial.print("Message arrived on topic: ");
  //Serial.print(topic);
  //Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    //Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  //Serial.println();
  if (String(topic) == "PUM2023v2/BETA/CTR") {
      start = clock();
      int DirectionV2 = 1;
      int DirectionO2 = 1;
      int Vlong = 0;
      int RelOmega;
      int RelValue;
      string VVal;
      string VOmega;
      string tempStrV;
      for (int i = 0; (((char)message[i]!=';')&&(i < length)); i++) {
      tempStrV[i]=(char)message[i];
      messageTemp += (char)message[i];
      Vlong++;
      }
      if(tempStrV[0]=='-'){
        DirectionV2 = -1;
        for (int i = 1; (i < Vlong); i++){
            VVal[i-1]=tempStrV[i];
        }
        //Serial.print( "Kierunkek V - ");
      }
      if(tempStrV[0]=='+'){
        DirectionV2 = 1;
        for (int i = 1; (i < Vlong); i++){
            VVal[i-1]=tempStrV[i];
        }
        //Serial.print( "Kierunkek V + ");
      }
      if((tempStrV[0]>=48)&&(tempStrV[0]<=57)){ 
        for (int i = 0; (i < Vlong); i++){
            VVal[i]=tempStrV[i];
        }
        //Serial.print( "Kierunkek V domyslny (+) ");
      }
      for (int i = 0; (i < Vlong-1); i++){
            if((VVal[i]<48)&&(VVal[i]>57)){
              VVal="0";
            }
      }
      RelValue = abs(stoi(VVal));
      //Serial.print( "\nWartosc V: \n");
      //Serial.print( RelValue);
      //Serial.print( "\n");
    
      string tempStrO;
      for (int i = Vlong+1; ((i < length)); i++) {
      tempStrO[i-(Vlong+1)]=(char)message[i];
      messageTemp += (char)message[i];
      }

      if(tempStrO[0]=='-'){
        DirectionO2 = -1;
        for (int i = 1; (i < (length-Vlong+1)); i++){
            VOmega[i-1]=tempStrO[i];
        }
       //Serial.print( "Kierunkek Omega - ");
      }
      if(tempStrO[0]=='+'){
        DirectionO2 = 1;
        for (int i = 1; (i < (length-Vlong+1)); i++){
            VOmega[i-1]=tempStrO[i];
        }
        //Serial.print( "Kierunkek Omega + ");
      }

      if((tempStrO[0]>=48)&&(tempStrO[0]<=57)){ 
        for (int i = 0; (i < (length-Vlong+1)); i++){
            VOmega[i]=tempStrO[i];
        }
        //Serial.print( "Kierunkek Omega domyslny (+) ");
      }  

      for (int i = 0; (i < (length-Vlong-2)); i++){
            if((VOmega[i]<48)&&(VOmega[i]>57)){
              VOmega="0";
            }
      }
    RelOmega = abs(stoi(VOmega));
    int _RMotorD, _LMotorD;
    int _RMotorPWM, _LMotorPWM;

    if (RelOmega <= 20) { 
        _RMotorD = DirectionV2 == 1 ? HIGH : LOW;  
        _LMotorD = DirectionV2 == 1 ? HIGH : LOW;
        
        if (DirectionO2>0) {
        _RMotorPWM = map(abs(RelValue - RelValue * 0.01* RelOmega), 0, 100, 0, 255);
        _LMotorPWM = map(abs(RelValue), 0, 100, 0, 255);
        }
        if (DirectionO2<0) {
        _RMotorPWM = map(abs(RelValue), 0, 100, 0, 255);
        _LMotorPWM = map(abs(RelValue - RelValue * 0.01* RelOmega), 0, 100, 0, 255);

        }
    } else { 
        _RMotorD = DirectionO2 == 1 ? HIGH : LOW;  
        _LMotorD = DirectionO2 == 1 ? LOW : HIGH; 
        _RMotorPWM = map(RelOmega, 20, 100, 130, 200);
        _LMotorPWM = map(RelOmega, 20, 100, 130, 200);
    }
      
      digitalWrite(TRIG_PIN, LOW);
      delayMicroseconds(2);
      digitalWrite(TRIG_PIN, HIGH);
      delayMicroseconds(10);
      digitalWrite(TRIG_PIN, LOW);
      long duration = pulseIn(ECHO_PIN, HIGH);
      long distance = (duration/2) / 29.1;
      //long distance = 25;
      if((distance>=STOPdistance)){
        setLeftMotor(_LMotorPWM,_LMotorD);
        setRightMotor(_RMotorPWM,_RMotorD);
      }
      else {
        stop();
      }
      //Serial.print( "\nL Value -  ");
      //Serial.print(_LMotorPWM);
      //Serial.print( "\nR Valiue -  ");
      //Serial.print(_RMotorPWM);
    }
  }
void setup() {
  Serial.begin(74880);
  pinMode(RightMotorPWM, OUTPUT);
  pinMode(RightMotorIN1, OUTPUT);
  pinMode(RightMotorIN2, OUTPUT);
  pinMode(LeftMotorPWM, OUTPUT);
  pinMode(LeftMotorIN1, OUTPUT);
  pinMode(LeftMotorIN2, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.print("\nSTART: ");
  stop();
  setup_wifi();
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    Serial.print("\nMQTT RECONNECTING! ");
    stop();
    connect_mqttServer();
  delay(500);
  }
  checkDistanceAndStop();
  client.loop();
  watch_dog();
}
