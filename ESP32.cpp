#include <WiFi.h>
#include <string>
#include <PubSubClient.h>

#define LMotorPWM 26 
#define LMotorD 27 
#define RMotorPWM 33 
#define RMotorD 25
#define ledPin 2

clock_t start ;
clock_t temp ;
clock_t czas;
long lastMsg = 0;
using namespace std;

//Local wifi ssid and password
const char* ssid ="wifi_ssid";   
const char* password = "wifi_password";
//Your MQTT server
const char* mqtt_server ="broker.mqttdashboard.com";
const char* mqtt_username = "ESPCTR0002";
const char* mqtt_password = "12345678";

const char* inTopic = "PUM2023/#";
const int mqtt_port =1883;

WiFiClient espClient;
PubSubClient client(espClient);

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
    blink_led(2,200); //blink LED twice (for 200ms ON time) to indicate that wifi not connected
    delay(1000); //
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
  // Loop until we're reconnected

  while (!client.connected()) {

    if(WiFi.status() != WL_CONNECTED){
      //if not connected, then first connect to wifi
      setup_wifi();
    }
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESPCTR0002";   // Create a random client ID
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");

      client.subscribe(inTopic);   // subscribe the topics here
      //client.subscribe(command2_topic);   // subscribe the topics here
    } else {
      temp = clock();
      czas = (double)(temp - start) / CLOCKS_PER_SEC;
       //Serial.print( "\nSince the last message:\n");
        if(czas>2){
         analogWrite(LMotorPWM, 0);
         analogWrite(RMotorPWM, 0);
        }
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");   // Wait 5 seconds before retrying
      blink_led(3,200);
      delay(3000);
    }
  }
}

//this function will be executed whenever there is data available on subscribed topics
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

  if (String(topic) == "PUM2023/CTR/TEST") {
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
      Serial.print((char)message[i]);
      tempStrV[i]=(char)message[i];
      messageTemp += (char)message[i];
      Vlong++;
      }
      if(tempStrV[0]=='-'){
        DirectionV2 = -1;
        for (int i = 1; (i < Vlong); i++){
            VVal[i-1]=tempStrV[i];
        }
        Serial.print( "Kierunkek V - ");
      }
      if(tempStrV[0]=='+'){
        DirectionV2 = 1;
        for (int i = 1; (i < Vlong); i++){
            VVal[i-1]=tempStrV[i];
        }
        Serial.print( "Kierunkek V + ");
      }
      if((tempStrV[0]>=48)&&(tempStrV[0]<=57)){ 
        for (int i = 0; (i < Vlong); i++){
            VVal[i]=tempStrV[i];
        }
        Serial.print( "Kierunkek V domyslny (+) ");
      }
      for (int i = 0; (i < Vlong-1); i++){
            if((VVal[i]<48)&&(VVal[i]>57)){
              VVal="0";
            }
      }
      
      RelValue = stoi(VVal);
      Serial.print( "\nWartosc V: \n");
      Serial.print( RelValue);
      Serial.print( "\n");
      
      string tempStrO;
      for (int i = Vlong+1; ((i < length)); i++) {
      //Serial.print((char)message[i]);
      tempStrO[i-(Vlong+1)]=(char)message[i];
      messageTemp += (char)message[i];
      }
      //Serial.print( "\n\n\n");

      if(tempStrO[0]=='-'){
        DirectionO2 = -1;
        for (int i = 1; (i < (length-Vlong+1)); i++){
            VOmega[i-1]=tempStrO[i];
        }
        Serial.print( "Kierunkek Omega - ");
      }
      if(tempStrO[0]=='+'){
        DirectionO2 = 1;
        for (int i = 1; (i < (length-Vlong+1)); i++){
            VOmega[i-1]=tempStrO[i];
        }
        Serial.print( "Kierunkek Omega + ");
      }

      if((tempStrO[0]>=48)&&(tempStrO[0]<=57)){ 
        for (int i = 0; (i < (length-Vlong+1)); i++){
            VOmega[i]=tempStrO[i];
        }
        Serial.print( "Kierunkek Omega domyslny (+) ");
      }  

      for (int i = 0; (i < (length-Vlong-2)); i++){
            if((VOmega[i]<48)&&(VOmega[i]>57)){
              VOmega="0";
            }
      }
      Serial.print( "\n");
      RelOmega = stoi(VOmega);
      Serial.print( "\nWartosc Omega:\n");
      Serial.print(RelOmega);
      Serial.print( "\n");

    if(DirectionV2==1){
      if(DirectionO2>0){
        float R = abs(RelValue * 2.55 - (RelValue * 0.01 * RelOmega * 2.55));
        float L = abs(RelValue * 2.55) ;
        analogWrite(LMotorPWM, (int)L);
        digitalWrite(LMotorD, LOW);
        analogWrite(RMotorPWM, (int)R);
        digitalWrite(RMotorD, LOW);
        Serial.print( "\nL: ");
        Serial.print( L);
        Serial.print( "\nR: ");
        Serial.print( R);
        Serial.print( "\n");
      }
      if(DirectionO2<0){
        float L = abs(RelValue * 2.55 - (RelValue * 0.01 * RelOmega * 2.55));
        float R = abs(RelValue * 2.55) ;
        analogWrite(LMotorPWM, (int)L);
        digitalWrite(LMotorD, LOW);
        analogWrite(RMotorPWM, (int)R);
        digitalWrite(RMotorD, LOW);
        Serial.print( "\nL: ");
        Serial.print( L);
        Serial.print( "\nR: ");
        Serial.print( R);
        Serial.print( "\n");
      }
      if(RelOmega==0){
        float L = abs(RelValue * 2.55);
        float R = abs(RelValue * 2.55) ;
        analogWrite(LMotorPWM, (int)L);
        digitalWrite(LMotorD, LOW);
        analogWrite(RMotorPWM, (int)R);
        digitalWrite(RMotorD, LOW);
        Serial.print( "\nL: ");
        Serial.print( L);
        Serial.print( "\nR: ");
        Serial.print( R);
        Serial.print( "\n");
      }
    }
    if(DirectionV2<0){
      if(DirectionO2>0){
        float L = abs(RelValue * 2.55 - (RelValue * 0.01 * RelOmega * 2.55));
        float R = abs(RelValue * 2.55) ;
        analogWrite(LMotorPWM, (int)L);
        digitalWrite(LMotorD, HIGH);
        analogWrite(RMotorPWM, (int)R);
        digitalWrite(RMotorD, HIGH);
        Serial.print( "\nL: ");
        Serial.print( L);
        Serial.print( "\nR: ");
        Serial.print( R);
        Serial.print( "\n");
      }
      if(DirectionO2<0){
        float R = abs(RelValue * 2.55 - (RelValue * 0.01 * RelOmega * 2.55));
        float L = abs(RelValue * 2.55) ;
        analogWrite(LMotorPWM, (int)L);
        digitalWrite(LMotorD, HIGH);
        analogWrite(RMotorPWM, (int)R);
        digitalWrite(RMotorD, HIGH);
        Serial.print( "\nL: ");
        Serial.print( L);
        Serial.print( "\nR: ");
        Serial.print( R);
        Serial.print( "\n");
      }
      if(RelOmega==0){
        float L = abs(RelValue * 2.55);
        float R = abs(RelValue * 2.55) ;
        analogWrite(LMotorPWM, (int)L);
        digitalWrite(LMotorD, HIGH);
        analogWrite(RMotorPWM, (int)R);
        digitalWrite(RMotorD, HIGH);
        Serial.print( "\nL: ");
        Serial.print( L);
        Serial.print( "\nR: ");
        Serial.print( R);
        Serial.print( "\n");
      }
    }
  }
}

void setup() {
  Serial.begin(921600);
  pinMode(LMotorPWM, OUTPUT);
  pinMode(LMotorPWM, OUTPUT);
  pinMode(RMotorPWM, OUTPUT);
  pinMode(LMotorD, OUTPUT);
  pinMode(RMotorD, OUTPUT);
  pinMode(ledPin, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);
}

void loop() {
  
  if (!client.connected()) {
    connect_mqttServer();
  }
  client.loop();
  
  long now = millis();
  if (now - lastMsg > 4000) {
    lastMsg = now;

    temp = clock();
    czas = (double)(temp - start) / CLOCKS_PER_SEC;
    //Serial.print( "\nSince the last message:\n");
    if(czas>2){
        analogWrite(LMotorPWM, 0);
        analogWrite(RMotorPWM, 0);
      }
    client.publish("PUM2023/CTR/Answer", "ok"); 
  }
}
