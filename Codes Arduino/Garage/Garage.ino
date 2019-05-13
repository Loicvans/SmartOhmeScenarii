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
const char* mqtt_topic_car = "garage/voiture";
const char* mqtt_topic_citerne = "garage/citerne";
const char* mqtt_topic_door = "garage/bigdoor";
// The client id identifies the ESP8266 device. Think of it a bit like a hostname (Or just a name, like Greg).
const char* clientID = "esp32_Garage";

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker

#include <ESP32Servo.h>

Servo servo;

int pinServoMoteur=19;
int redpin = 2; //select the pin for the red LED
int greenpin =4;// select the pin for the green LED
int bluepin =16; // select the pin for the  blue LED

//Water lvl tank
// const int pin_water_tank = 27;
const int led_water_lvl_low = 25;
const int led_water_lvl_medium = 26;
const int led_water_lvl_high = 14;

// Pin water sensor
#define ANALOG_PIN_0 33


int val;

#include <NewPing.h>

#define TRIGGER_PIN 18  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN 5  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.


// Infos à envoyer 

int garage_door_open = 1;
int car_present = 1;
int tank_pourcent = 0;



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




    pinMode(redpin, OUTPUT); pinMode(bluepin, OUTPUT); pinMode(greenpin, OUTPUT); Serial.begin(9600);

   servo.attach(pinServoMoteur);

//Water lvl tank
  pinMode(led_water_lvl_low , OUTPUT); 
  pinMode(led_water_lvl_medium , OUTPUT);
  pinMode(led_water_lvl_high , OUTPUT); 
  






}

void loop() {

  delay(500);  // Wait 500ms between pings (about 2 pings/sec). 29ms should be the shortest delay between pings.
  unsigned int uS = sonar.ping(); // Send ping, get ping time in microseconds (uS).
  Serial.print("Ping: ");
  Serial.print(uS / US_ROUNDTRIP_CM); // Convert ping time to distance and print result (0 = outside set distance range, no ping echo)
  Serial.println("cm");
  
  if (uS / US_ROUNDTRIP_CM <10){ /**VERT**/
digitalWrite(redpin, HIGH);
digitalWrite(greenpin, LOW);
digitalWrite(bluepin, LOW);
garage_door_open = 0;
car_present = 1;
 
servo.write(0);               
delay(1000);
}
  if(uS / US_ROUNDTRIP_CM==0)
digitalWrite(redpin, LOW);{
digitalWrite(greenpin, LOW);
digitalWrite(bluepin, LOW);}

  if(10 < uS / US_ROUNDTRIP_CM && uS / US_ROUNDTRIP_CM< 27){ /**ROUGE**/
digitalWrite(redpin, LOW);
digitalWrite(greenpin, HIGH);
digitalWrite(bluepin, LOW);
 servo.write(90);           
  delay(1000);
  garage_door_open = 1;
  car_present = 0;
}

  if( uS / US_ROUNDTRIP_CM >=27){ /**BLEU**/
digitalWrite(redpin, LOW);
digitalWrite(greenpin, LOW);
digitalWrite(bluepin, HIGH);
servo.write(90);           
delay(1000);
garage_door_open = 1;
car_present = 0;
}


Serial.println(val, DEC);



//Water lvl tank
  double tank_value = analogRead(ANALOG_PIN_0);
  Serial.println(tank_value);
  tank_pourcent = 0;

  if(tank_value >1000){

    tank_pourcent = 33;
    digitalWrite(led_water_lvl_low , HIGH);
    
    if(tank_value >1900){

      digitalWrite(led_water_lvl_medium , HIGH); //allumer la Led    
      tank_pourcent = 66;
      
      if(tank_value >2150){
        tank_pourcent = 100;
        digitalWrite(led_water_lvl_high , HIGH); //allumer la Led
      }
      else{
        digitalWrite(led_water_lvl_high , LOW); //allumer la Led
        
      } 
    }
    else{
      digitalWrite(led_water_lvl_medium , LOW); //allumer la Led
    }    
  }
  else{
     digitalWrite(led_water_lvl_low , LOW); //allumer la Led
     
  }
  delay(100);

  // Variables to send 

  char door[16];
  char car[16];
  char tank[16];
  
  itoa(garage_door_open, door, 10);
  itoa(car_present, car, 10);
  itoa(tank_pourcent, tank, 10);

  // Send door state 

  
  if (client.publish(mqtt_topic_door, door)){
    Serial.println(door);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_door, "Button pressed!");
  }


  // Send car

  if (client.publish(mqtt_topic_car, car)){
    Serial.println(car);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_car, "Button pressed!");
  }

  // Send tank level

  if (client.publish(mqtt_topic_citerne, tank)){
    Serial.println(tank);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_citerne, "Button pressed!");
  }

  delay(500);

}
