#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

#include "arduino_secrets.h" 

#define SENSOR 5
#define RED 6
#define GREEN 7
#define BLUE 8

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

WiFiClient wifiClient;
MqttClient mqttClientPublish(wifiClient);
MqttClient mqttClientSubscriber(wifiClient);

const char broker[] = "test.mosquitto.org";
int        port     = 1883;
const char topicTemp[]  = "arduino/temp";
const char topicHum[]  = "arduino/hum";
const char topicLED[] = "arduino/led";
int temp, hum;
String messageMQTT;

DHT dht (SENSOR, DHT22);

void setup() {

  Serial.begin(9600);
  dht.begin();

  //pins for LED RGB
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  // Check for the version of firmware
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
    changeLedColorByString("red");
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network: ");
  printCurrentNet();
  changeLedColorByString("green");

  // Conect to MQTT Broker
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClientPublish.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClientPublish.connectError());
    changeLedColorByString("purple");
    while (1);
  }
  if (!mqttClientSubscriber.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClientSubscriber.connectError());
    changeLedColorByString("purple");
    while (1);
  }
  Serial.println("You're connected to the MQTT broker!");
  subscribe(topicLED);

  changeLedColorByString("green");

}

void loop() {
  //mqttClientPublish.poll();
  //mqttClientSubscriber.poll();
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  delay(2000);
  sendMessage(topicHum, hum);
  sendMessage(topicTemp, temp);
  getMessage(mqttClientSubscriber);

  if (messageMQTT != ""){
    StaticJsonDocument<200> json;
    DeserializationError error = deserializeJson(json, messageMQTT);
    if (error) {
      Serial.println(F("is not a JSON"));
      changeLedColorByString(messageMQTT);
    }else{
      int redValue = json["r"];
      int greenValue = json["g"];
      int blueValue = json["b"];
      changeLedColorByJson(redValue, greenValue, blueValue);
    }
    messageMQTT = "";
    json = "";
  }
}

void printCurrentNet() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void subscribe(char const* tpc){
  Serial.print("Subscribing to topic: ");
  Serial.println(tpc);
  
  mqttClientSubscriber.subscribe(tpc);
  Serial.println("Waiting for messages on topic: ");
}

void getMessage(MqttClient mqttSubscriber){
  int messageSize = mqttSubscriber.parseMessage();
  if (messageSize) {
    Serial.print("Received a message with topic '");
    Serial.print(mqttSubscriber.messageTopic());
    Serial.print("', length ");
    Serial.print(messageSize);
    Serial.println(" bytes:");
    char message[100] = "";
    int i = 0;
    
    while (mqttSubscriber.available()) {
      message[i] = ((char)mqttSubscriber.read());
      i++;
    }
    String messageParseToString(message);
    messageMQTT = messageParseToString;
    Serial.print(messageParseToString);
    Serial.println();
  }
}

void changeLedColorByString(String color){
  if (color == "red" || color == "r" || color == "rojo"){
    analogWrite(RED, 255);
    analogWrite(GREEN, 0);
    analogWrite(BLUE, 0);
  }else{
    if (color == "blue" || color == "b" || color == "azul"){
      analogWrite(RED, 0);
      analogWrite(GREEN, 0);
      analogWrite(BLUE, 255);
    }else{
      if (color == "green" || color == "g" || color == "verde"){
        analogWrite(RED, 0);
        analogWrite(GREEN, 255);
        analogWrite(BLUE, 0);
      }else{
        if (color == "yellow" || color == "y" || color == "amarillo"){
        analogWrite(RED, 255);
        analogWrite(GREEN, 255);
        analogWrite(BLUE, 0);
        }else{
          if (color == "purple" || color == "p" || color == "lila" || color == "morado"){
            analogWrite(RED, 255);
            analogWrite(GREEN, 0);
            analogWrite(BLUE, 255);
          }else{
            if (color == "white" || color == "w" || color == "blanco"){
              analogWrite(RED, 255);
              analogWrite(GREEN, 255);
              analogWrite(BLUE, 255);
            }else{
              analogWrite(RED, 0);
              analogWrite(GREEN, 0);
              analogWrite(BLUE, 0);
            }
          }
        }
      }
    }
  }
}
void changeLedColorByJson(int redValue, int greenValue, int blueValue){
  if(redValue == NULL){
    redValue = 0;
  }
  if(greenValue == NULL){
    greenValue = 0;
  }
  if(blueValue == NULL){
    blueValue = 0;
  }
  analogWrite(RED, redValue);
  analogWrite(GREEN, greenValue);
  analogWrite(BLUE, blueValue);
}

void sendMessage(char const* tpc, int msg){
  // Publish
  Serial.print("Sending message to topic: ");
  Serial.println(tpc);
  Serial.print(msg);

  // send message, the Print interface can be used to set the message contents
  mqttClientPublish.beginMessage(tpc);
  mqttClientPublish.print(msg);
  mqttClientPublish.endMessage();

  Serial.println();
}