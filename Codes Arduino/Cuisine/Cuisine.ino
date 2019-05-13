/* Dans le cadre du projet Smart Ohm'e de BA3 ingénieur civil en informatique et gestion
 *  AFENZOUAR Farid 
 *  email : Farid.AFENZOUAR@student.umons.ac.be
 *  FISICARO Federico
 *  email: Federico.FISICARO@student.umons.ac.be
 *  NGUYEN Harry
 *  email : Harry.NGUYEN@student.umons.ac.be
 *  VANSNICK Loïc
 *  email : Loic.VANSNICK@student.umons.ac.be
 */
 
#include <WiFi.h> // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker

// WiFi
// Make sure to update this for your own WiFi network!
const char* ssid = "IoTWifi";
const char* wifi_password = "SmartBuilding";

// MQTT
const char* mqtt_server = "192.168.4.1";
const char* mqtt_topic_people = "cuisine/people";
const char* mqtt_topic_temperature = "cuisine/temperature";
const char* mqtt_topic_temp_ref = "cuisine/refTemp";
const char* mqtt_topic_window = "cuisine/window";
// The client id identifies the ESP8266 device. Think of it a bit like a hostname (Or just a name, like Greg).
const char* clientID = "esp32_Cuisine";

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker

// Données à envoyées en plus

int fenetre_ouverte = 0;

// Pour les capteurs

#include "DHT.h"
#define DHTPIN 26
#define DHTTYPE DHT22

#include <ESP32Servo.h>

Servo servo;
DHT dht(DHTPIN, DHTTYPE);

int temp_ref=0;
int ecart_temp=2;

int persons = 0;
int Sortir = 0; 
int Entrer = 0;

int pinInfraRouge1=16;
int pinInfraRouge2=21;
int pinLed=25;
int pinServo=13;




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
  
// setup pour les capteurs

  pinMode(pinInfraRouge1, INPUT);// set pin as input
 pinMode(pinInfraRouge2, INPUT);
 pinMode(pinLed, OUTPUT);
 servo.attach(pinServo);
 dht.begin();
 
 if (isnan(dht.readTemperature())) {
  Serial.println("ERREUR REF");
  } else{
    temp_ref = dht.readTemperature();
    }

}

void loop() {

  int t = dht.readTemperature();

  for(int i = 0; i < 5; i++){ // Pour envoyer les données toutes les demis secondes

  
  t = dht.readTemperature(); 
  if (isnan(t)) 
  {
    Serial.println("Erreur lecture!");
  }
  else
  {
    Serial.print("Ref:");
    Serial.print(temp_ref);
    Serial.print(" \t Temperatura: ");
    Serial.print(t);
    Serial.println(" *C");
  }


// Ouvre et ferme la fenetre


  if(t>temp_ref+ecart_temp){
    servo.write(120);
    fenetre_ouverte = 1;
    } else{
      servo.write(0);
      fenetre_ouverte = 0;
      }

 
  int detect1 = digitalRead(pinInfraRouge1);
  int detect2 = digitalRead(pinInfraRouge2);
  


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
  delay(100);
  Serial.print("Personnes");
  Serial.println(persons);
 // Serial.print("Sortir");
 // Serial.println(Sortir);
 // Serial.print("Entrer");
 // Serial.println(Entrer); 

 

  if(persons>0){
    digitalWrite(pinLed, HIGH);
    
    }
    else {
    digitalWrite(pinLed,LOW);
    }

    
  // Envoie des données toutes les demis secondes

  }

  char temper[16];
  char window_state[16];
  char person_room[16];
  char reference[16];

  itoa(t, temper, 10);
  itoa(fenetre_ouverte, window_state, 10);
  itoa(persons, person_room, 10);
  itoa(temp_ref, reference, 10);
  

  // Envoi de la température

  if (client.publish(mqtt_topic_temperature, temper)){
    Serial.println(temper);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_temperature, "Button pressed!");
  }

  // Envoi de la position de la fenetre

  if (client.publish(mqtt_topic_window, window_state)){
    Serial.println(window_state);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_window, "Button pressed!");
  }

  // Envoi PresenceDetection

  if (client.publish(mqtt_topic_people, person_room)){
    Serial.println(person_room);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_people, "Button pressed!");
  }

  // Envoi du seuil de la température
  
  if (client.publish(mqtt_topic_temp_ref, reference)){
    Serial.println(reference);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_temp_ref, "Button pressed!");
  }

}

