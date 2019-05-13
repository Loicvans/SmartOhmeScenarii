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
const char* mqtt_topic_light = "salon/light";
const char* mqtt_topic_window = "salon/window";
const char* mqtt_topic_rain = "salon/rain";
// The client id identifies the ESP8266 device. Think of it a bit like a hostname (Or just a name, like Greg).
const char* clientID = "esp32_Salon";

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker


// Pins des capteurs

int pinLED = 32 ;
int pinServoMoteur1= 12;
int pinServoMoteur2= 22;
// int pinPhotoResistor= 25;
// int pinWaterLevel = 13; 


// Pin photoresistor

#define ANALOG_PIN_0 33
int analog_value_light = 0;

int voltageRef=0;

// Pin water sensor
#define ANALOG_PIN_7 35

//Servomoteurs
#include <ESP32Servo.h>
Servo servo1;
Servo servo2;
int angle1=0; //Fenêtre droite ouverte
int angle2=90; //Fenêtre gauche ouverte



// Infos à envoyer

int lum_allumees = 1;
int fenetres_ouvertes = 0;
int detect_eau = 1;

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

  pinMode(pinLED, OUTPUT);
  Serial.print("initialisation");


  int LuminositeReference = analogRead(ANALOG_PIN_0);
  voltageRef = LuminositeReference * (1.25 / 1023.0);


  servo1.attach(pinServoMoteur1);
  servo1.write(angle1);
  servo2.attach(pinServoMoteur2);
  servo2.write(angle2);
}

void loop() {

  
  analog_value_light = analogRead(ANALOG_PIN_0);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = analog_value_light * (1.25 / 1023.0);
  Serial.print("voltage ");
  Serial.print(voltage);

  Serial.print("  volatge référence ");
  Serial.print(voltageRef);
  if (voltage <= voltageRef) {
    Serial.print("  Lumiere éteinte ");
    digitalWrite(pinLED, LOW);
    lum_allumees = 0;
  } else {
    Serial.print("  Lumiere allumée ");
    digitalWrite(pinLED, HIGH);
    lum_allumees = 1;
  }

  //Capteur du niveau d'eau
  double waterlevel = analogRead(ANALOG_PIN_7);
  Serial.print("Niveau d'eau ");
  Serial.println(waterlevel);

  if(waterlevel>150){ //S'il y a de l'eau
 
  servo1.write(90);
  servo2.write(0);
  delay(3000);
  fenetres_ouvertes = 0;
  detect_eau = 1;
  
  }

  else{
     
  servo1.write(0);
  servo2.write(90);
  fenetres_ouvertes = 1;
  detect_eau = 0;
  }

  
  // Sending variables

  char lum[16];
  char fen[16];
  char raincheck[16];
  
  itoa(lum_allumees, lum, 10);
  itoa(fenetres_ouvertes, fen, 10);
  itoa(detect_eau, raincheck, 10);

  // Send light state
  
  if (client.publish(mqtt_topic_light, lum)){
    Serial.println(lum);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_light, "Button pressed!");
  }


  // Send window state
  
  if (client.publish(mqtt_topic_window, fen)){
    Serial.println(fen);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_window, "Button pressed!");
  }

  // Send rain check 

  if (client.publish(mqtt_topic_rain, raincheck)){
    Serial.println(raincheck);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_rain, "Button pressed!");
  }
  

  delay(1000);

}
