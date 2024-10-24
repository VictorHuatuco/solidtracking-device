#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include "colors.h"
#include "IoTicosSplitter.h"

String dId = "123456";
String webhook_pass = "6rOrOGEsaO";
String webhook_endpoint = "http://192.168.18.114:3001/api1/getdevicecredentials";
const char *mqtt_server = "192.168.18.114";



#define led 2

//WiFi
const char *wifi_ssid = "FamiliaHV-2.4G";
const char *wifi_password = "Fam.HV20034744";

//Functions definitions
void clear();
bool get_mqtt_credentials();
void check_mqtt_connection();
bool reconnect();

// Global Vars
WiFiClient espclient;
PubSubClient client(espclient);
DynamicJsonDocument mqtt_data_doc(2048);
long lastReconnectAttemp = 0;

void setup() {

  Serial.begin(921600);
  pinMode(led, OUTPUT);
  clear();
  Serial.print(underlinePurple + "\n\n\nWiFi Connection in Progress" + fontReset + Purple);
  WiFi.begin(wifi_ssid, wifi_password);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
    counter++;
    if(counter > 10){
      Serial.print("  ⤵" + fontReset);
      Serial.print(Red + "\n\n         Ups WiFi Connection Failed :( ");
      Serial.println(" -> Restarting..." + fontReset);
      delay(2000);
      ESP.restart();
    }
  }
    Serial.print("  ⤵" + fontReset);
  //Printing local ip
  Serial.println(boldGreen + "\n\n         WiFi Connection -> SUCCESS :)" + fontReset);
  Serial.print("\n         Local IP -> ");
  Serial.print(boldBlue);
  Serial.print(WiFi.localIP());
  Serial.println(fontReset);
  
}

void loop() {
  check_mqtt_connection();
}

bool reconnect(){
  if (!get_mqtt_credentials()){
    Serial.println(boldRed + "\n\n      Error getting mqtt credentials :( \n\n RESTARTING IN 10 SECONDS");
    Serial.println(fontReset);
    delay(10000);
    ESP.restart();
    return false;
  }
  //Setting up Mqtt Server
  client.setServer(mqtt_server, 1883);
  Serial.print(underlinePurple + "\n\n\nTrying MQTT Connection" + fontReset + Purple + "  ⤵");
  String str_client_id = "device_" + dId + "_" + random(1,9999);
  const char* username = mqtt_data_doc["username"];
  const char* password = mqtt_data_doc["password"];
  String str_topic = mqtt_data_doc["topic"];

  if (username == nullptr || password == nullptr || str_topic == "") {
    Serial.println(boldRed + "\n\n      Error: Missing MQTT credentials. Restarting..." + fontReset);
    delay(10000);
    ESP.restart();
  }

  if(client.connect(str_client_id.c_str(), username, password)){
    Serial.print(boldGreen + "\n\n         Mqtt Client Connected :) " + fontReset);
    delay(2000);
    client.subscribe((str_topic + "+/actdata").c_str());
    return true;
  }else{
    Serial.print(boldRed + "\n\n         Mqtt Client Connection Failed :( " + fontReset);
    return false;
  }

}

void check_mqtt_connection(){
  
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(Red + "\n\n         Ups WiFi Connection Failed :( ");
    Serial.println(" -> Restarting..." + fontReset);
    delay(15000);
    ESP.restart();
  }

  if(!client.connected()){
    
    long now = millis();
    if (now - lastReconnectAttemp > 5000){
      lastReconnectAttemp = millis();
       if(reconnect()){
         lastReconnectAttemp = 0;
       }
    }
  }else{
    client.loop();
  }



}


bool get_mqtt_credentials(){
  Serial.print(underlinePurple + "\n\n\nGetting MQTT Credentials from WebHook" + fontReset + Purple + "  ⤵");
  delay(1000);
  String toSend = "dId=" + dId + "&password=" + webhook_pass;
  HTTPClient http;
  http.begin(webhook_endpoint);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int response_code = http.POST(toSend);
  if(response_code < 0 ){
    Serial.print(boldRed + "\n\n         Error Sending Post Request :( " + fontReset);
    http.end();
    return false;
  }
  if(response_code != 200){
    Serial.print(boldRed + "\n\n         Error in response :(   e-> "  + fontReset + " " + response_code);
    http.end();
    return false;
  }
  if (response_code == 200){
    String responseBody = http.getString();
    Serial.print(boldGreen + "\n\n         Mqtt Credentials Obtained Successfully :) " + fontReset);
    deserializeJson(mqtt_data_doc, responseBody);
    http.end();
    delay(1000);
  }
  return true;
}


void clear()
{
  Serial.write(27);    // ESC command
  Serial.print("[2J"); // clear screen command
  Serial.write(27);
  Serial.print("[H"); // cursor to home command
}