
/*
 * MQTTClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>
#include<String.h>

// WIFI libs
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>


// I2C libs
#include <Hash.h>
#include <Wire.h>

#define USE_SERIAL Serial
// I2C for comms to arduino IO
#define I2CAddressESPWifi 8
#define NUM_SLOTS 4

#define I2CRQST 32


ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient espClient;

const char* mqttServer = "192.168.1.6"; 
const int   mqttPort = 1883; 
const char* mqttUser = "mikes"; 
const char* mqttPassword = "mqpass";

char baystat[4];
char numstr[20];
char msg[MQTT_MAX_PACKET_SIZE+1];
int ledOnCount = 0;
int MQTTError = 0;

//String MQTTMessage = "";

/*************************** Sketch Code ************************************/

//unsigned int baystat = 0;
unsigned long oldnow = millis();
unsigned long ledTimer, oldLedTimer = millis();
bool  ledState = HIGH;

// MQTT Client
PubSubClient client(espClient);


  void setup() {
  	// USE_SERIAL.begin(921600);
  	USE_SERIAL.begin(115200);
  
  	//Serial.setDebugOutput(true);
  	USE_SERIAL.setDebugOutput(true);
  
  	USE_SERIAL.println();
  	USE_SERIAL.println();
  	USE_SERIAL.println();
  
    USE_SERIAL.println(F("MQTT demo"));
     int msglen = MQTT_MAX_PACKET_SIZE;
     USE_SERIAL.print("max packet length");
     USE_SERIAL.println(msglen,DEC);
     
    pinMode(BUILTIN_LED, OUTPUT);  
     
    // Have to use esp8266 as an I2C master but Arduino can also be a master to control displays etc
    Wire.begin();//Change to Wire.begin() for non ESP.
    digitalWrite(BUILTIN_LED,LOW);
    
  	for(uint8_t t = 4; t > 0; t--) {
  		USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
  		USE_SERIAL.flush();
  		delay(1000);
  	}

  
      WiFiMulti.addAP("VodafoneConnect96376469", "58xdlm9ddipa8dh");   // add Wi-Fi networks you want to connect to
      WiFiMulti.addAP("Mike's iPhone SE", "MjKmJe6360");

      // see if the user wants to add a wifi password etc
      
      while(WiFiMulti.run() != WL_CONNECTED) {
          delay(500);
          USE_SERIAL.print(".");
      }
  
      if(MDNS.begin("esp8266")) {
          USE_SERIAL.println("MDNS responder started");
      }
      
      client.setServer(mqttServer, 1883);
      client.setCallback(callback);
      // connect to MQTT using reconnect() in main loop
      
      /*
      if (client.connect("arduinoClient", "mikes", "mqpass")) {
        USE_SERIAL.println("MQTT connected to broker @" + mqttServer);
        
        client.publish("carpark","Smart Car Park Ready");
        client.subscribe("news");
        
        USE_SERIAL.println("MQTT publishing carpark data");
        USE_SERIAL.println("MQTT subscribed to news");
    }
    else{
       USE_SERIAL.println("MQTT cannot connect to broker @" + mqttServer);
    }
    */
    
    //digitalWrite(BUILTIN_LED,HIGH);
    
    // initialise carpak bay data
    baystat[0] = 0;
    baystat[1] = 0;
    baystat[2] = 0;
    baystat[3] = 0;
  
  }
  
  void loop() {
    unsigned long now = millis();
    delay(1);

    if (!client.connected()) {
      reconnect();
    }
    
    client.loop();
    
    // send message to server every x seconds
    if( (now - oldnow) > 2000){

      int count1 = Wire.requestFrom(8,4);
      USE_SERIAL.print("get response");
      USE_SERIAL.println(count1,DEC);
      
      USE_SERIAL.print("values [ ");
      int n = 0;
      while(Wire.available())
      {
        char c = Wire.read();
        if(n < NUM_SLOTS){
          baystat[n] = c;
          n++;
        }
       }
       
       for(int n=0;n<NUM_SLOTS;n++){
        Serial.print(baystat[n],DEC); 
       }

       USE_SERIAL.println("]");

       snprintf(msg,MQTT_MAX_PACKET_SIZE,"{\"car park\": \"busy street\",{\"bay\": %2d,\"bay\": %2d,\"bay\": %2d,\"bay\": %2d}}",baystat[0],baystat[1],baystat[2],baystat[3]);
       USE_SERIAL.println(msg);
       client.publish("carpark",msg);
       delay(10);
        // reset the timeout
        oldnow = now;
    }
       
  }
  
  // WiFi MQTT received message handler
  void callback(char* topic, byte* payload, unsigned int length)
  {
    MQTTError = 2;  // take priority and flash led slowly for a few seconds
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is ON)
    USE_SERIAL.print("Message in topic: ");
    USE_SERIAL.print("Control Message: " + String(length) + "bytes received on topic: ");
    USE_SERIAL.println(topic);
    USE_SERIAL.print("Message: ");

    Wire.beginTransmission(I2CAddressESPWifi);
      for(int i = 0; i < length; i++) {
        USE_SERIAL.print((char)payload[i]);
        Wire.write(payload[i]);
    }
    Wire.endTransmission();
    USE_SERIAL.println();
  }

  void reconnect() {
     
      // Loop until we're reconnected
      while (!client.connected()) {
        USE_SERIAL.print("Attempting MQTT connection...");
        
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
      
        if (client.connect(clientId.c_str(),"mikes","mqpass")) {
          USE_SERIAL.println("connected");
          // Once connected, publish an announcement...
          client.publish("carpark", "Street 1 Ready");
          // ... and resubscribe to the incoming channel
          client.subscribe("news");
        } else {
          USE_SERIAL.print("failed, reconnect=");
          USE_SERIAL.print(client.state());
          USE_SERIAL.println(" try again in 5 seconds");
          // Wait 5 seconds before retrying
          delay(5000);
        }
      }
}

