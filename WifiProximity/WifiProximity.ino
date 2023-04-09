#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Ubidots.h>
#include "./esppl_functions.h"

// Configeration (Enter your own values in these fields)

int ledPin = D5;                                // Pin Number where led bulb is connected

char ubidotsToken[] = "";

// MQTT Broker Topic
const char *mactopic = "";          // MQTT topic for subscribing to mac address list
const char *statustopic = "";    // MQTT topic to publish rssi of nearby devices

// WiFi Details
const char *ssid = "";                    // Your Wifi Username
const char *password = "";              // Your Wifi Password

// MQTT Broker Details
const char *mqtt_broker = "broker.hivemq.com";  // MQTT Broker URL
const int mqtt_port = 1883;                     // MQTT Port


// Global Variables (To be used in program execution)
int numMacs = 0;
uint8_t (*mac_address)[ESPPL_MAC_LEN];
char (*mac_string)[20];
const String client_id = "esp8266-client-"+String(WiFi.macAddress());

WiFiClient espClient;                           // Wifi Client
PubSubClient mqttClient(espClient);             // MQTT Client
Ubidots ubidotsClient(ubidotsToken, UBI_HTTP);  // Ubidots Client

StaticJsonDocument<400> macList;                // Json document to receive mac address list
StaticJsonDocument<200> deviceRssi;             // Json document to store result data
JsonObject result = deviceRssi.to<JsonObject>();// Json object to access document

// Utility function to compare two mac address
bool maccmp(uint8_t *mac1, uint8_t *mac2)
{
    for (int i = 0; i < ESPPL_MAC_LEN; i++)
        if (mac1[i] != mac2[i])
            return false;
    return true;
}

// callback function called whenever we get a new wifi packet in promiscuous mode
void cb(esppl_frame_info *info)
{
    for (int i = 0; i < numMacs; i++)
    {
        if (maccmp(info->sourceaddr, mac_address[i]) || maccmp(info->receiveraddr, mac_address[i]))
        {
            String idx = String(i);
            digitalWrite(ledPin, HIGH);

            if(result.containsKey(idx))
              result[idx] = max((int)result[idx], info->rssi);
            else
              result[idx] = info->rssi;

            Serial.printf("\n%d is here! :)", i);
            Serial.printf(", RSSI : %d", info->rssi);
            Serial.println();
        }

    }
}

// Utility function to convert string representation of mac address to an array
void StringToMac(const char* str, uint8_t mac[6]){
   sscanf(str, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

// callback function called whenever we get a list of mac-address from MQTT
void callback(char *topic, byte *payload, unsigned int length) {
  DeserializationError error = deserializeJson(macList, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  // dynamically allocate size if number of mac-address is changed
  if(macList.size() != numMacs){
    numMacs = macList.size();
    if(mac_address != nullptr){
      delete[] mac_address;
      delete[] mac_string;
    }
    mac_address = new uint8_t[numMacs][ESPPL_MAC_LEN];
    mac_string = new char[numMacs][20];
  }
  
  // Process the list of all mac-address received
  for(int i = 0; i < numMacs; ++i){
    strcpy(mac_string[i], macList[i]);
    StringToMac(macList[i], mac_address[i]);
    for(int j = 0; j < 6; ++j){
      Serial.print(mac_address[i][j], HEX);
      if(j != 5)
        Serial.print(":");
    }
    Serial.println();
  }
}


// Setup NodeMCU device
void setup()
{
    Serial.begin(115200);
    esppl_init(cb);                        

    // connecting to a WiFi network
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to the WiFi network");

    //connecting to a mqtt broker
    mqttClient.setServer(mqtt_broker, mqtt_port);
    mqttClient.setCallback(callback);

    while (!mqttClient.connected()) {
        Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
        
        if (mqttClient.connect(client_id.c_str())) {
            Serial.println("Public emqx mqtt broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.println(mqttClient.state());
            delay(2000);
        }
    }
    mqttClient.subscribe(mactopic);
    pinMode(ledPin, OUTPUT);
}



void loop()
{
    
    // Wait to get a updated list of mac-address from MQTT broker
    do {
      for(int i = 0; i < 10; ++i){
        mqttClient.loop();
        delay(1000);
      }
    } while(numMacs == 0);


    // Repeatedly iterate over all Wifi channel to sniff packets and compare their mac with our list
    esppl_sniffing_start();
    for(int times = 0; times < 100; ++times){
      for (int i = ESPPL_CHANNEL_MIN; i <= ESPPL_CHANNEL_MAX; i++)
      {
          esppl_set_channel(i);
          while (esppl_process_frames());
      }
    }
    esppl_sniffing_stop();
    digitalWrite(ledPin, LOW);

    // Reconnect to wifi    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("-");
    }
    Serial.println();

    // Reconnect to MQTT broker
    while (!mqttClient.connected()) {
        
        if (mqttClient.connect(client_id.c_str())) {
            mqttClient.subscribe(mactopic);
        } else {
            Serial.print("failed with state ");
            Serial.println(mqttClient.state());
            delay(2000);
        }
    }

    for(int i = 0; i < numMacs; ++i){
      String idx = String(i);
      ubidotsClient.add(mac_string[i], result.containsKey(idx));
    }

    bool bufferSent = false;
    bufferSent = ubidotsClient.send();

    // Send any result to MQTT broker
    if(result.size() != 0){
      Serial.println("Uploading results!");

      String output;
      serializeJson(deviceRssi, output);
      mqttClient.publish(statustopic, output.c_str());
      deviceRssi.clear();
      result = deviceRssi.to<JsonObject>();
    }
}