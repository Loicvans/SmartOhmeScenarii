#include <WiFi.h> // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker

// WiFi
// Make sure to update this for your own WiFi network!
const char* ssid = "IoTWifi";
const char* wifi_password = "SmartBuilding";

// MQTT
const char* mqtt_server = "192.168.4.1";
const char* mqtt_topic_humidity = "chambre/humidity";
const char* mqtt_topic_temperature = "chambre/temperature";
const char* mqtt_topic_color_led = "chambre/light_color";

// The client id identifies the ESP8266 device. Think of it a bit like a hostname (Or just a name, like Greg).
const char* clientID = "esp32_Chambre";

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker



// Pour les capteurs

#include <LiquidCrystal_I2C.h>
// #include <SimpleDHT.h>

#include "DHT.h"
#define DHTPIN 5     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);


// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

//int pinDHT11 = 5;
//SimpleDHT11 dht11(pinDHT11);


#include "SevSeg.h"
SevSeg sevseg;

#include <Wire.h>
#include "DS1307.h"

/* Rétro-compatibilité avec Arduino 1.x et antérieur */
#if ARDUINO >= 100
#define Wire_write(x) Wire.write(x)
#define Wire_read() Wire.read()
#else
#define Wire_write(x) Wire.send(x)
#define Wire_read() Wire.receive()
#endif


/** Fonction de conversion BCD -> decimal */
byte bcd_to_decimal(byte bcd) {
  return (bcd / 16 * 10) + (bcd % 16); 
}

/** Fonction de conversion decimal -> BCD */
byte decimal_to_bcd(byte decimal) {
  return (decimal / 10 * 16) + (decimal % 10);
}


/** 
 * Fonction récupérant l'heure et la date courante à partir du module RTC.
 * Place les valeurs lues dans la structure passée en argument (par pointeur).
 * N.B. Retourne 1 si le module RTC est arrêté (plus de batterie, horloge arrêtée manuellement, etc.), 0 le reste du temps.
 */
byte read_current_datetime(DateTime_t *datetime) {
  
  /* Début de la transaction I2C */
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire_write((byte) 0); // Lecture mémoire à l'adresse 0x00
  Wire.endTransmission(); // Fin de la transaction I2C
 
  /* Lit 7 octets depuis la mémoire du module RTC */
  Wire.requestFrom(DS1307_ADDRESS, (byte) 7);
  byte raw_seconds = Wire_read();
  datetime->seconds = bcd_to_decimal(raw_seconds);
  datetime->minutes = bcd_to_decimal(Wire_read());
  byte raw_hours = Wire_read();
  if (raw_hours & 64) { // Format 12h
    datetime->hours = bcd_to_decimal(raw_hours & 31);
    datetime->is_pm = raw_hours & 32;
  } else { // Format 24h
    datetime->hours = bcd_to_decimal(raw_hours & 63);
    datetime->is_pm = 0;
  }
  datetime->day_of_week = bcd_to_decimal(Wire_read());
  datetime->days = bcd_to_decimal(Wire_read());
  datetime->months = bcd_to_decimal(Wire_read());
  datetime->year = bcd_to_decimal(Wire_read());
  
  /* Si le bit 7 des secondes == 1 : le module RTC est arrêté */
  return raw_seconds & 128;
}


/** 
 * Fonction ajustant l'heure et la date courante du module RTC à partir des informations fournies.
 * N.B. Redémarre l'horloge du module RTC si nécessaire.
 */
void adjust_current_datetime(DateTime_t *datetime) {
  
  /* Début de la transaction I2C */
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire_write((byte) 0); // Ecriture mémoire à l'adresse 0x00
  Wire_write(decimal_to_bcd(datetime->seconds) & 127); // CH = 0
  Wire_write(decimal_to_bcd(datetime->minutes));
  Wire_write(decimal_to_bcd(datetime->hours) & 63); // Mode 24h
  Wire_write(decimal_to_bcd(datetime->day_of_week));
  Wire_write(decimal_to_bcd(datetime->days));
  Wire_write(decimal_to_bcd(datetime->months));
  Wire_write(decimal_to_bcd(datetime->year));
  Wire.endTransmission(); // Fin de transaction I2C
}

int LEDR = 2;
int LEDJ = 23;
int LEDB = 15;

int LEDON = 0;




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

  dht.begin();
  
// initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
  Serial.println("Temp and hum");

   byte numDigits = 4;
   byte digitPins[] = {19, 18, 13, 4};
   byte segmentPins[] = {27, 26, 33, 16, 17, 14, 25, 32};
   sevseg.begin(COMMON_ANODE, numDigits, digitPins, segmentPins);
  //Set the desired brightness (0 to 100);
  sevseg.setBrightness(90);


  /* Initialise le port I2C */
  Wire.begin();
  
  /* Vérifie si le module RTC est initialisé */
  DateTime_t now;
  if (read_current_datetime(&now)) {
    Serial.println(F("L'horloge du module RTC n'est pas active !"));
    
    // Reconfiguration avec une date et heure en dure (pour l'exemple)
    now.seconds = 0;
    now.minutes = 47;
    now.hours = 14; // 12h 0min 0sec
    now.is_pm = 1; 
    now.day_of_week = 2;
    now.days = 23; 
    now.months = 04;
    now.year = 19; // 1 dec 2016
    adjust_current_datetime(&now);
  }
  pinMode(LEDR, OUTPUT);
  pinMode(LEDJ, OUTPUT);
  pinMode(LEDB, OUTPUT);
}

void loop() {

  DateTime_t now;
  if (read_current_datetime(&now)) {
    Serial.println(F("L'horloge du module RTC n'est pas active !"));
  }

  /* Affiche la date et heure courante */
 /* Serial.print(F("Date : "));
  Serial.print(now.days);
  Serial.print(F("/"));
  Serial.print(now.months);
  Serial.print(F("/"));
  Serial.print(now.year + 2000);
  Serial.print(F("  Heure : "));
  Serial.print(now.hours);
  Serial.print(F(":"));
  Serial.print(now.minutes);
  Serial.print(F(":"));
  Serial.println(now.seconds);
  Serial.println();
*/
  int heure = int(now.seconds);
  if (heure < 40){
    if (heure > 20){
      digitalWrite(LEDJ, LOW);
      digitalWrite(LEDR, LOW);
      digitalWrite(LEDB, HIGH);
      LEDON = 2;
    } else {
      digitalWrite(LEDJ, HIGH);
      digitalWrite(LEDR, LOW);
      digitalWrite(LEDB, LOW);
      LEDON = 1;
    }
  }
  if (heure > 40){
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDJ, LOW);
    digitalWrite(LEDB, LOW);
    LEDON = 3;
  }
  sevseg.setNumber(heure, 2); //Displays '3.141'
  for (int i = 0; i < 1000; i ++){
    sevseg.refreshDisplay();
    delay(1);
  }
 // sevseg.refreshDisplay(); // Must run repeatedly
  /* Rafraichissement une fois par seconde */ 

  
  if (heure == 59){
 heure=0;
  }


  int humidity = dht.readHumidity();
  int temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
    /*
    // read without samples.
    byte temperature = 0;
    byte humidity = 0;
    int err = SimpleDHTErrSuccess;
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      Serial.print("Read DHT11 failed, err="); Serial.println(err);delay(1000);
      return;
    }
    //Serial.print((int)temperature); Serial.print(" *C, "); 
    Serial.print((int)humidity); Serial.println(" H");
    */
    /*
    Serial.print((int)temperature); Serial.print(" *C, ");
    Serial.print((int)humidity); Serial.println(" H");
    */
    // set cursor to first column, first row
    lcd.setCursor(0, 0);
    // print message
    lcd.clear();
    lcd.print("T ");
    lcd.print((int)temperature);
    lcd.print(" *C ");
    lcd.print("H ");
    lcd.print((int)humidity);
    lcd.print(" % ");
    
   // set cursor to first column, second row
    lcd.setCursor(0,1);
    if((int)temperature <= 23){
      lcd.setCursor(0,1);
      lcd.print("Mettez une veste");
    }
    if((int)temperature >= 24){
      lcd.setCursor(0,1);
      lcd.print("Mettez T-shirt");
    } 
    
   // delay(1500);
    // clears the display to print new message
  //  lcd.clear();
  

  // apres les autres trucs

  char LED[16];
  char temp[16];
  char hum[16]; //to transform the int in string
  itoa(LEDON, LED, 10);
  itoa(temperature, temp, 10);
  itoa(humidity, hum, 10);


  // Envoi de l'humidité
  
  if (client.publish(mqtt_topic_humidity, hum)){
    Serial.println(hum);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_humidity, "Button pressed!");
  }


  if (client.publish(mqtt_topic_temperature, temp)){
    Serial.println(temp);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_temperature, "Button pressed!");
  }


  if (client.publish(mqtt_topic_color_led, LED)){
    Serial.println(LED);
    // client.publish(mqtt_topic, cstr);
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
  }else {
        Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(clientID);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(mqtt_topic_color_led, "Button pressed!");
  }

  // delay(2000);

}
