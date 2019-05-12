#include <WiFi.h> // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker

// WiFi
// Make sure to update this for your own WiFi network!
const char* ssid = "IoTWifi";
const char* wifi_password = "SmartBuilding";

// MQTT
const char* mqtt_server = "192.168.4.1";
const char* mqtt_topic_mail = "hall/courrier";
const char* mqtt_topic_people = "hall/personnes";
const char* mqtt_topic_garden = "hall/jardin";
// The client id identifies the ESP8266 device. Think of it a bit like a hostname (Or just a name, like Greg).
const char* clientID = "esp32_Hall";

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker

// Sensors


#include "SevSeg.h"
#include <ESP32Servo.h>

SevSeg sevseg;
Servo servo;
int button_pin = 35; 
int IR1=23;
int IR2=22;
int servo_pin=25;
int light= 27;
int post_light=21;
int post_pin= 18;

int persons=0;
int Sortir = 0; 
int Entrer = 0;


// Données à envoyer 

int courrier = 0;
int jardin = 0;


void setup() {

  
  Serial.begin(9600);
  delay(1000); // give me time to bring up serial monitor
  Serial.println("ESP32 Analog IN Test");
  
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to the WiFi
  WiFi.begin(ssid, wifi_password);

  // Wait until the connection has been confirmed before continuing
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Debugging - Output the IP Address of the ESP8266
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  if (client.connect(clientID)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }
  
// autres setup à faire

  byte numDigits = 1;
  byte digitPins[] = {};
  // byte segmentPins[] = {13, 12, 16, 4, 15, 26, 14, 17};
  byte segmentPins[] = {13, 33, 16, 4, 15, 26, 14, 17};
  bool resistorsOnSegments = true;
  sevseg.begin(COMMON_ANODE, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(90);

  pinMode(light , OUTPUT); 

  pinMode(button_pin, INPUT);
  servo.attach(servo_pin);
  servo.write(0);
  
  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);

  pinMode(post_pin, INPUT);
  pinMode(post_light , OUTPUT); 
  Serial.begin(9600);

  digitalWrite(light,HIGH);

  
}

void loop() {

  int detect1 = digitalRead(IR1);
  int detect2 = digitalRead(IR2);
  int button = digitalRead(button_pin);
  int angle = servo.read();
  int post= digitalRead(post_pin);

  if(post==HIGH){
    digitalWrite(post_light, HIGH);
    courrier = 1;
    }
  else{
    digitalWrite(post_light, LOW);
    courrier = 0;
  }

  
  
  if(button==LOW ){

    if(angle==0){
      for(angle = 0; angle <= 105; angle++)  
      {                                  
        servo.write(angle);               
        delay(15);                   
      }   
      delay(1000);
      }
      else{
       for(angle = 105; angle >= 0; angle--)  
      {                                  
        servo.write(angle);               
        delay(15);                   
      }   
      delay(1000);
        }
  }
    

   if (detect1 == LOW  && detect2 == HIGH){
      Entrer = 1;

      if(Sortir==1){
        persons--;
        Entrer=0;
        Sortir=0;
        
    delay(3000);
      }    
    }
    
  if (detect2 == LOW && detect1 == HIGH){
      //Serial.println("Détecter2");
      Sortir = 1;

      if(Entrer == 1){
        persons++;
        Entrer=0;
        Sortir=0;
        if(persons<0) {persons =0;}
     delay(3000);
      }
    }
  delay(10);
  if(persons < 0){
    persons = 0;
  }
  sevseg.setNumber(persons);
  sevseg.refreshDisplay(); 

  if(persons ==0 && angle != 0){

    for(angle = 105; angle >= 0; angle--)    
    {                                
      servo.write(angle);           
      delay(15);       
    }
    
    }
  //Serial.println(angle);
  //Serial.println(persons);

  if (angle == 0){
    jardin = 0;
  } else {
    jardin = 1;
  }
  //Serial.println(garden);
  // if the value is a sentence/word 
  
  char people[16];
  char mail[16];
  char garden[16];
  
  itoa(courrier, mail, 10);
  itoa(persons, people, 10);
  itoa(jardin, garden, 10);

  // Send mail detection

  
  if (client.publish(mqtt_topic_mail, mail)){
    Serial.println(mail);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_mail, "Button pressed!");
  }

  // Send person number

  if (client.publish(mqtt_topic_people, people)){
    Serial.println(people);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_people, "Button pressed!");
  }


  // Send door status

  
  if (client.publish(mqtt_topic_garden, garden)){
    Serial.println(garden);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_garden, "Button pressed!");
  }

  delay(100);
  
}
