/*
* ESP8266_Test.ino - Simple sketch to listen for E1.31 data on an ESP8266 
*                    and print some statistics.
*
* Project: E131 - E.131 (sACN) library for Arduino
* Copyright (c) 2015 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

#include <ESP8266WiFi.h>
#include <ESPAsyncE131.h>
#include <my92xx.h>
#include <ArduinoOTA.h> 
#include <PubSubClient.h> 


#define MY92XX_MODEL    MY92XX_MODEL_MY9231     // The MY9291 is a 4-channel driver, usually for RGBW lights
#define MY92XX_CHIPS    2                       // No daisy-chain
#define MY92XX_DI_PIN   12                      // DI GPIO
#define MY92XX_DCKI_PIN 14                      // DCKI GPIO
#define UNIVERSE 1                      // First DMX Universe to listen for
#define UNIVERSE_COUNT 1                // Total number of Universes to listen for, starting at UNIVERSE

my92xx _my92xx = my92xx(MY92XX_MODEL, MY92XX_CHIPS, MY92XX_DI_PIN, MY92XX_DCKI_PIN, MY92XX_COMMAND_DEFAULT);
ESPAsyncE131 e131(UNIVERSE_COUNT);
WiFiClient espClient;
PubSubClient client(espClient);

//USER CONFIGURED SECTION START//
#define MQTT_CLIENT_NAME    "UniqueClientName" 
const char* ssid = "TaitWiFi";
const char* password = "93e0cb0614";
const char* mqtt_server = "192.168.86.168";
const int mqtt_port = 1883;
const char *mqtt_user = "Rob";
const char *mqtt_pass = "stanger127";
//USER CONFIGURED SECTION END//

int r = 0;
int g = 0;
int b = 0;
int w = 0;
uint8_t r_show = 0;
uint8_t g_show = 0;
uint8_t b_show = 0;
uint8_t w_show = 0;
int brightness = 255;
bool boot = true;
String effect = "MQTT";
char charPayload[50];
const char *mqtt_client_name = MQTT_CLIENT_NAME; 


void setup_wifi() 
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() 
{
  int retries = 0;
  while (!client.connected()) {
    if(retries < 5)
    {
      Serial.print("Attempting MQTT connection...");
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass)) 
      {
        Serial.println("connected");
        if(boot == false)
        {
          client.publish(MQTT_CLIENT_NAME "/checkIn", "Reconnected"); 
        }
        if(boot == true)
        {
          client.publish("checkIn/B1_2", "Rebooted");
          boot = false;
        }
        client.subscribe("B1_2/power");
        client.subscribe("B1_2/color");
        client.subscribe("B1_2/brightness");
        client.subscribe("B1_2/white");
        client.subscribe("B1_2/effect");
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        delay(5000);
      }
    }
    if(retries > 5)
    {
    ESP.restart();
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  int intPayload = newPayload.toInt();
  Serial.println(newPayload);
  Serial.println();
  newPayload.toCharArray(charPayload, newPayload.length() + 1); 
  if (newTopic == "B1_2/color")
  {
    client.publish("B1_2/colorState", charPayload); 
    // get the position of the first and second commas
    uint8_t firstIndex = newPayload.indexOf(',');
    uint8_t lastIndex = newPayload.lastIndexOf(',');
    
    uint8_t rgb_red = newPayload.substring(0, firstIndex).toInt();
    if (rgb_red < 0 || rgb_red > 255) {
      return;
    } else {
      r = rgb_red;
      Serial.println(r);
    }
    
    uint8_t rgb_green = newPayload.substring(firstIndex + 1, lastIndex).toInt();
    if (rgb_green < 0 || rgb_green > 255) {
      return;
    } else {
      g = rgb_green;
      Serial.println(g);
    }
    
    uint8_t rgb_blue = newPayload.substring(lastIndex + 1).toInt();
    if (rgb_blue < 0 || rgb_blue > 255) {
      return;
    } else {
      b = rgb_blue;
      Serial.println(b);
    }
    updateLights();
  }
  if (newTopic == "B1_2/brightness")
  {
    client.publish("B1_2/brightnessState", charPayload); 
    brightness = intPayload;
    updateLights();
  }
  if (newTopic == "B1_2/white")
  {
    client.publish("B1_2/whiteState", charPayload); 
    w = intPayload;
    updateLights();
  }
  if (newTopic == "B1_2/effect")
  {
    client.publish("B1_2/effectState", charPayload); 
    effect = newPayload;
    updateLights();
  }
  if (newTopic == "B1_2/power") 
  {
    client.publish("B1_2/powerState", charPayload); 
    if (newPayload == "ON")
    {
      updateLights();
    }
    if (newPayload == "OFF")
    {
      _my92xx.setChannel(0, 0);
      _my92xx.setChannel(1, 0);
      _my92xx.setChannel(2, 0); 
      _my92xx.setChannel(3, 0); 
      _my92xx.setChannel(4, 0); 
      _my92xx.setChannel(5, 0); 
      _my92xx.setChannel(6, 0); 
      _my92xx.setState(true);
      _my92xx.update();
    }
  }
}

void updateLights()
{
      r_show = (r * brightness / 255);
      g_show = (g * brightness / 255);
      b_show = (b * brightness / 255);
      w_show = (w * brightness / 255);
      _my92xx.setChannel(4, r_show);
      _my92xx.setChannel(3, g_show);
      _my92xx.setChannel(5, b_show); 
      _my92xx.setChannel(0, w_show); 
      _my92xx.setChannel(1, w_show); 
      _my92xx.setState(true);
      _my92xx.update();
}


void setup() {
    Serial.begin(115200);
    delay(10);
    
    // Make sure you're in station mode    
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    setup_wifi();
    Serial.println("");
    Serial.print(F("Connecting to "));
    Serial.print(ssid);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    ArduinoOTA.setHostname("B1_2");
    ArduinoOTA.begin(); 
    
    // Choose one to begin listening for E1.31 data
    if (e131.begin(E131_UNICAST))                               // Listen via Unicast
    //if (e131.begin(E131_MULTICAST, UNIVERSE, UNIVERSE_COUNT))   // Listen via Multicast
        Serial.println(F("Listening for data..."));
    else 
        Serial.println(F("*** e131.begin failed ***"));

}

void loop() {
  if(effect == "E131")
  {
    if (!e131.isEmpty()) {
        e131_packet_t packet;
        e131.pull(&packet);     // Pull packet from ring buffer
        
        Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
                htons(packet.universe),                 // The Universe for this packet
                htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                e131.stats.num_packets,                 // Packet counter
                e131.stats.packet_errors,               // Packet error counter
                packet.property_values[1]);             // Dimmer data for Channel 1
        r_show = packet.property_values[1];
        g_show = packet.property_values[2];
        b_show = packet.property_values[3];
        w_show = packet.property_values[4];
    }

    _my92xx.setChannel(4, r_show); 
    _my92xx.setChannel(3, g_show); 
    _my92xx.setChannel(5, b_show); 
    _my92xx.setChannel(1, w_show); 
    _my92xx.setChannel(2, w_show); 
    _my92xx.setState(true);
    _my92xx.update();
  }
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
}
