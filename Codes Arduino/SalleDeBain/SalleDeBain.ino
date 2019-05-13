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
// Make sure to update this for your own MQTT Broker!
//const char* mqtt_server = "192.168.0.50";
const char* mqtt_server = "192.168.4.1";
const char* mqtt_topic_sdb_water = "sdb/waterlevel";
const char* mqtt_topic_sdb_hum = "sdb/humidity";
// The client id identifies the ESP8266 device. Think of it a bit like a hostname (Or just a name, like Greg).
const char* clientID = "esp32_SdB";

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker


byte HumidityReference=0;
int ecartHumidity=3; //Différence d'humidité pour que le ventilateur s'active

int waterlevelRef = 0;
int ecartWater=200; //Différence du niveau d'eau pour que le buzzer s'active


#include <SimpleDHT.h>

// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
int pinDHT11 = 32;
SimpleDHT11 dht11(pinDHT11);

// Declaration des variables

#define ANALOG_PIN_7 35
int pinVentilo = 18;
int pinLaser = 18;
int pinBuzzer = 15;


// Données à envoyer

int hum_send = 0;
int waterlvl_send = 0;

void setup() {
  // put your setup code here, to run once:
  
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

  
  pinMode(pinLaser, OUTPUT);
  pinMode(pinVentilo, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);

  //Prendre la temperature et l'humidité de référence
  byte temperatureRef = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperatureRef, &HumidityReference, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.println(err);delay(1000);
    return;
  }

  //Prendre le niveau d'eau de référence 
  //double waterlevelRef = analogRead(ANALOG_PIN_7);
}


void loop() {
  // put your main code here, to run repeatedly:

  //Capteur température et humidité
  
  
    
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.println(err);delay(1000);
    return;
  }
  
  Serial.print((int)temperature); Serial.print(" *C, "); 
  Serial.print((int)humidity); Serial.println(" H");
  hum_send = (int)humidity;
  Serial.print((int)HumidityReference); Serial.println(" H de référence");



  if(humidity > (int)HumidityReference+ecartHumidity){
    digitalWrite(pinLaser, HIGH);
    digitalWrite(pinVentilo, HIGH);
    }
    else { 
    digitalWrite(pinLaser, LOW);
    digitalWrite(pinVentilo, LOW);
      }
  
  // DHT11 sampling rate is 1HZ.
  delay(1500);



  //Capteur niveau d'eau
  double waterlevel = analogRead(ANALOG_PIN_7);
  int waterlvl_send = (int)waterlevel;
  Serial.println(waterlevelRef); 
  delay(100);


  // Sending data to mqtt server

  char hum[16];
  char water[16];
  itoa(waterlvl_send, water, 10);
  itoa(hum_send, hum, 10);

  Serial.println(hum);
  Serial.println(water);

  // Send hum 
  
  if (client.publish(mqtt_topic_sdb_hum, hum)){
    Serial.print("Humidite = ");
    Serial.println(hum);
    
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_sdb_hum, "Button pressed!");
  }
  
  // Send water level
  
  if (client.publish(mqtt_topic_sdb_water, water)){
    Serial.print("Water level is = ");
    Serial.println(water);
   
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_sdb_water, "Button pressed!");
  }

  
  //Passive Buzzer

  if (waterlvl_send>waterlevelRef+ecartWater){
 unsigned char i, j ;
 Serial.println("Buzz");
  
    for (i = 0; i <80; i++) // When a frequency sound
    {
      digitalWrite (pinBuzzer, HIGH) ; //send tone
      delay (1) ;
      digitalWrite (pinBuzzer, LOW) ; //no tone
      delay (1) ;
    }
    for (i = 0; i <100; i++) // When a frequency sound
    {
      digitalWrite (pinBuzzer, HIGH) ; //send tone
      delay (2) ;
      digitalWrite (pinBuzzer, LOW) ; //no tone
      delay (2) ;
    }
  }

}
