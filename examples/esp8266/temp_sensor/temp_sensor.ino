#include <PubSubClient.h>

/**
 * IoT Kit - Temperature and Humidity Logger
 * Author: Shawn Hymel (SparkFun Electronics)
 * Date: October 30, 2016
 * 
 * Log temperature and humidity data to a channel on
 * thingspeak.com once every 20 seconds.
 * 
 * Connections:
 *   Thing Dev |  RHT03
 *  -----------|---------
 *      3V3    | 1 (VDD) 
 *        4    | 2 (DATA)
 *      GND    | 4 (GND)
 *      
 * Development environment specifics:
 *  Arduino IDE v1.6.5
 *  Distributed as-is; no warranty is given.  
 */

#define NODEMCU_D0 16
#define NODEMCU_D1 5
#define NODEMCU_D2 4
#define NODEMCU_D3 0
#define NODEMCU_D4 2
#define NODEMCU_D5 14
#define NODEMCU_D6 12
#define NODEMCU_D7 13
#define NODEMCU_D8 15
#define NODEMCU_D9 3
#define NODEMCU_D10 1
#define NODEMCU_D12 10

#include <ESP8266WiFi.h>
#include <SparkFun_RHT03.h>
#include <PubSubClient.h>

// CHANGE THESE VALUES FOR YOUR INSTALLATION!
// WiFi and Channel parameters
const char WIFI_SSID[] = "<your 2.4Ghz SSID>";
const char WIFI_PSK[] = "<your SSID password>";
const char* mqtt_server = "<your mqtt broker>";

// Pin definitions
const int RHT03_DATA_PIN = 4;
const int LED_PIN = 5; //16;  // was 5

// Global variables
WiFiClient client;
PubSubClient pubSubClient(client);
RHT03 rht;

char tempCStr[7];
char tempFStr[7];
char humiStr[7];

char mac_addr[18];

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Set up LED for debugging
  pinMode(LED_PIN, OUTPUT);

  // Connect to WiFi
  connectWiFi();

  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

  pubSubClient.setServer(mqtt_server, 1883);
  //pubSubClient.setCallback(callback);

  // Call rht.begin() to initialize the sensor and our data pin
  rht.begin(RHT03_DATA_PIN);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  pubSubClient.loop();

  // Flash LED to show that we're sampling
  digitalWrite(LED_PIN, LOW);

  // Call rht.update() to get new humidity and temperature values from the sensor.
  int updateRet = rht.update();

  // If successful, the update() function will return 1.
  if (updateRet == 1)  {
     publishTemps();
  } else  {
    // If the update failed, try delaying for some time
    delay(RHT_READ_INTERVAL_MS);
  }

  // Turn LED off when we've posted the data
  digitalWrite(LED_PIN, HIGH);

  // ThingSpeak will only accept updates every 15 seconds
  delay(20000);
}

void publishTemps(){
    // The tempC(), tempF(), and humidity() functions can be 
    // called after a successful update()
    float temp_c = rht.tempC();
    float temp_f = rht.tempF();
    float humidity = rht.humidity();

    char tempTopic[100];
    char rhtTopic[150];
    snprintf(tempTopic, 99, "sensors/temp/%s/%3.2f", mac_addr, temp_f);
    snprintf(rhtTopic, 149, "sensors/rht/%s/%3.2f/%3.2f", mac_addr, temp_f, humidity);
    
    Serial.print("publishing topic: ");
    Serial.println(tempTopic);
    pubSubClient.publish(tempTopic, "");
    Serial.print("publishing topic: ");
    Serial.println(rhtTopic);
    pubSubClient.publish(rhtTopic, "");
}

// Attempt to connect to WiFi
void connectWiFi() {

  byte led_status = 0;

  // Set WiFi mode to station (client)
  WiFi.mode(WIFI_STA);

  // Initiate connection with SSID and PSK
  WiFi.begin(WIFI_SSID, WIFI_PSK);

  // Blink LED while we wait for WiFi connection
  while ( WiFi.status() != WL_CONNECTED ) {
    digitalWrite(LED_PIN, led_status);
    led_status ^= 0x01;
    delay(100);
  }

  // Turn LED off when we are connected
  digitalWrite(LED_PIN, HIGH);
}

void reconnect() {
  // Loop until we're reconnected
  while (!pubSubClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = mac_addr; //"ESP8266Client-";
    //clientId += "-";
    //clientId += String(random(0xffff), HEX);
    Serial.print(clientId);
    Serial.print("...");
    // Attempt to connect
    if (pubSubClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      pubSubClient.publish("outTopic", "hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(pubSubClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


