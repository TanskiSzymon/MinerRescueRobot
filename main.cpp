#include <WiFi.h>
#include <string>
#include <PubSubClient.h>
#include <string>
#include <iostream>

#define LMotorPWM 26 
#define LMotorD 27 
#define RMotorPWM 33 
#define RMotorD 25
#define ledPin 2
#define TRIG_PIN 32
#define ECHO_PIN 35

clock_t start ;
clock_t temp ;
clock_t czas;
long lastMsg = 0;
long lastMsgD = 0;
using namespace std;

//Local wifi ssid and password
const char* ssid ="SSID";   
const char* password = "PASSWORD";
//Your MQTT server
const char* mqtt_server ="broker.mqttdashboard.com";
const char* mqtt_username = "ESP32_CAR2";
const char* mqtt_password = "12345678";
const char* inTopic = "MQTT_PILOT/CTR";
const int mqtt_port =1883;
const int STOPdistance = 25; //cm
WiFiClient espClient;
PubSubClient client(espClient);

void checkDistanceAndStop() {
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
  string num_str = to_string(distance);

  if(distance < STOPdistance) {
    analogWrite(LMotorPWM, 0);
    analogWrite(RMotorPWM, 0);
    client.publish("MQTT_PILOT/ANSWER", "TO CLOSE"); 
   }
  }
}

void watch_dog(){
  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

    temp = clock();
    czas = (double)(temp - start) / CLOCKS_PER_SEC;
    if(czas>2){
        analogWrite(LMotorPWM, 0);
        analogWrite(RMotorPWM, 0);
        client.publish("MQTT_PILOT/ANSWER", "Watch_dog stop!"); 
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
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int c=0;
  while (WiFi.status() != WL_CONNECTED) {
    blink_led(2,200); 
    Serial.print(".");
    c=c+1;
    if(c>10){
        ESP.restart(); //restart ESP after 10 seconds
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void connect_mqttServer() {
  while (!client.connected()) {

    if(WiFi.status() != WL_CONNECTED){
      setup_wifi();
    }
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP_CAR2";   // Create a random client ID
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");

      client.subscribe(inTopic);   // subscribe the topics here
    } else {
      temp = clock();
      czas = (double)(temp - start) / CLOCKS_PER_SEC;
        if(czas>2){
         analogWrite(LMotorPWM, 0);
         analogWrite(RMotorPWM, 0);
        }
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");  
      blink_led(3,200);
      delay(2000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "MQTT_PILOT/CTR") {
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
      }
      if(tempStrV[0]=='+'){
        DirectionV2 = 1;
        for (int i = 1; (i < Vlong); i++){
            VVal[i-1]=tempStrV[i];
        }
      }
      if((tempStrV[0]>=48)&&(tempStrV[0]<=57)){ 
        for (int i = 0; (i < Vlong); i++){
            VVal[i]=tempStrV[i];
        }
      }
      for (int i = 0; (i < Vlong-1); i++){
            if((VVal[i]<48)&&(VVal[i]>57)){
              VVal="0";
            }
      } 
      RelValue = stoi(VVal);
      
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
      }
      if(tempStrO[0]=='+'){
        DirectionO2 = 1;
        for (int i = 1; (i < (length-Vlong+1)); i++){
            VOmega[i-1]=tempStrO[i];
        }
      }

      if((tempStrO[0]>=48)&&(tempStrO[0]<=57)){ 
        for (int i = 0; (i < (length-Vlong+1)); i++){
            VOmega[i]=tempStrO[i];
        }
      }  

      for (int i = 0; (i < (length-Vlong-2)); i++){
            if((VOmega[i]<48)&&(VOmega[i]>57)){
              VOmega="0";
            }
      }
      RelOmega = stoi(VOmega);
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
      if((distance>=STOPdistance)||((_RMotorD==HIGH)&&(_RMotorD==HIGH))){
        analogWrite(LMotorPWM, _LMotorPWM);
        analogWrite(RMotorPWM, _RMotorPWM);
        digitalWrite(LMotorD, _LMotorD);
        digitalWrite(RMotorD, _RMotorD);
      }
      else {
        analogWrite(LMotorPWM, 0);
        analogWrite(RMotorPWM, 0);
        digitalWrite(LMotorD, LOW);
        digitalWrite(RMotorD, LOW);
      }
      Serial.print( "\nL Value -  ");
      Serial.print(_LMotorPWM);
      Serial.print( "\nR Valiue -  ");
      Serial.print(_RMotorPWM);
    }
  }
void setup() {
  Serial.begin(921600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LMotorPWM, OUTPUT);
  pinMode(RMotorPWM, OUTPUT);
  pinMode(LMotorD, OUTPUT);
  pinMode(RMotorD, OUTPUT);
  pinMode(ledPin, OUTPUT);
  blink_led(1, 3000);
  setup_wifi();
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);
  blink_led(2, 500);
}

void loop() {
  if (!client.connected()) {
    analogWrite(LMotorPWM, 0);
    analogWrite(RMotorPWM, 0);
    connect_mqttServer();
  }
  client.loop();
  checkDistanceAndStop();
  watch_dog();
  delay(250);
}
