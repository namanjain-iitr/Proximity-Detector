#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "./esppl_functions.h"

const char *mactopic = ""; // MQTT mac-address list retrival topic
const char *statustopic = ""; // MQTT status update topic

// WiFi
const char *ssid = ""; // Enter your WiFi name
const char *password = "";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "broker.hivemq.com";
const int mqtt_port = 1883;

int numMacs = 0;
uint8_t (*mac_address)[6];
const String client_id = "esp8266-client-"+String(WiFi.macAddress());

WiFiClient espClient;
PubSubClient client(espClient);

StaticJsonDocument<400> doc;
StaticJsonDocument<200> status;
JsonObject result = status.to<JsonObject>();

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

            if(result.containsKey(idx)){
              result[idx] = max((int)result[idx], info->rssi);
            } else {
              result[idx] = info->rssi;
            }

            Serial.printf("\n%d is here! :)", i);
            Serial.printf("\nRSSI : %d", info->rssi);
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
  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  // dynamically allocate size if number of mac-address is changed
  if(doc.size() != numMacs){
    numMacs = doc.size();
    if(mac_address != nullptr){
      delete[] mac_address;
    }
    mac_address = new uint8_t[numMacs][ESPPL_MAC_LEN];
  }
  
  // Process the list of all mac-address received
  for(int i = 0; i < numMacs; ++i){
    StringToMac(doc[i], mac_address[i]);
    for(int j = 0; j < 6; ++j){
      Serial.print(mac_address[i][j], HEX);
      if(j != 5)
        Serial.print(":");
    }
    Serial.println();
  }
}


// start variables package - Skickar 2018 hardware LED for NodeMCU on mini breadboard //
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
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);

    while (!client.connected()) {
        Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
        
        if (client.connect(client_id.c_str())) {
            Serial.println("Public emqx mqtt broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.println(client.state());
            delay(2000);
        }
    }
    client.subscribe(mactopic);
}



void loop()
{
    
    // Wait to get a updated list of mac-address from MQTT broker
    do {
      for(int i = 0; i < 10; ++i){
        client.loop();
        delay(1000);
      }
    } while(numMacs == 0);


    // Repeatedly iterate over all Wifi channel to sniff packets and compare their mac with our list
    esppl_sniffing_start();
    for(int times = 0; times < 100; ++times){
      for (int i = ESPPL_CHANNEL_MIN; i <= ESPPL_CHANNEL_MAX; i++)
      {
          esppl_set_channel(i);
          while (esppl_process_frames())
          {
              //
          }
      }
    }
    WiFi.disconnect();
    esppl_sniffing_stop();

    // Reconnect to wifi    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("-");
    }
    Serial.println();

    // Reconnect to MQTT broker
    while (!client.connected()) {
        
        if (client.connect(client_id.c_str())) {
            client.subscribe(mactopic);
        } else {
            Serial.print("failed with state ");
            Serial.println(client.state());
            delay(2000);
        }
    }

    // Send any result to MQTT broker
    if(result.size() != 0){
      Serial.println("Uploading results!");
      String output;
      serializeJson(status, output);
      client.publish(statustopic, output.c_str());
      status.clear();
      result = status.to<JsonObject>();
    }
}