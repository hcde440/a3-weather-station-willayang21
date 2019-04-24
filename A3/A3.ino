

#include <ESP8266WiFi.h>    //Requisite Libraries . . .
#include "Wire.h"           //
#include <PubSubClient.h>   //
#include <ArduinoJson.h>    //
#include <Adafruit_Sensor.h>
#include <Adafruit_MPL115A2.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_SSD1306.h>

// initialize mpl115a2 sensor
Adafruit_MPL115A2 mpl115a2;

// oled display
Adafruit_SSD1306 oled = Adafruit_SSD1306(128, 32, &Wire);

// pin connected to DH22 data line
#define DATA_PIN 12

// create DHT22 instance
DHT_Unified dht(DATA_PIN, DHT22);

//#define wifi_ssid "University of Washington"  
//#define wifi_password "" 

#define wifi_ssid "NETGEAR38"   
#define wifi_password "imaginarysky003" 

#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "..."               //this is its server login, unique to the server
#define mqtt_password "..."           //this is it server password, unique to the server

WiFiClient espClient;             //blah blah blah, espClient
PubSubClient mqtt(espClient);     //blah blah blah, tie PubSub (mqtt) client to WiFi client

char mac[6]; //A MAC address is a 'truly' unique ID for each device

char message[201]; // 201, as last character in the array is the NULL character, denoting the end of the array

#define BUTTON_PIN 5
bool current = false; // button state
bool last = false; // button state
unsigned long timerOne; // timer
bool one = true; // to ensure if statement doesn't jump
bool two = false; // to ensure if statement doesn't jump
bool three = false; // to ensure if statement doesn't jump

void setup() {
  pinMode(BUTTON_PIN, INPUT);
  mpl115a2.begin(); //  starts sensing pressure
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  oled.display(); // make the screen display
  // start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  while(! Serial);

  // initialize dht22
  dht.begin();

  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback); //register the callback function
    // text display tests
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  timerOne = millis();
  bool one = true;
  bool two = false;
  bool three = false;
}


void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }

  mqtt.loop(); //this keeps the mqtt connection 'active'
  
  oled.clearDisplay(); // clear display
  oled.setCursor(0,0); // set cursor

  if (millis() - timerOne > 5000 && one) { // every five sec, and ensure the if statement goes one by one
    float celsius_MPL = 0;     
    celsius_MPL = mpl115a2.getTemperature();  // start the sensor
    Serial.print("Reading from MPL: "); Serial.print(celsius_MPL, 1); Serial.println(" *C");
    char str_temp[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character
    //take temp, format it into 5 char array with a decimal precision of 2, and store it in str_temp
    dtostrf(celsius_MPL, 5, 2, str_temp);
    sprintf(message, "{\"temp\":\"%s\"}", str_temp);
    mqtt.publish("willa/Temp", message); 
    // print temperature reading from DHT
    oled.print("Temp: "); oled.print(celsius_MPL,0); oled.println(" *C ");
    oled.display();  
    timerOne = millis();
    one = false; 
    two = true; // next is two
  }

  if (millis() - timerOne > 5000 && two) { // another five sec, and ensure the if statement goes one by one 
    char str_pres[6];
    float pressureKPA = 0;      
    pressureKPA = mpl115a2.getPressure();  // get the pressure
    Serial.print("Pressure (kPa): "); Serial.print(pressureKPA, 4); Serial.println(" kPa");
    dtostrf(pressureKPA, 5, 2, str_pres);
    sprintf(message, "{\"pres\":\"%s\"}", str_pres);
    mqtt.publish("willa/Pres", message);  
    oled.print("Pres: "); oled.print(pressureKPA,0); oled.println(" kPa ");      
    oled.display();  
    timerOne = millis();
    three = true; // next is three
    two = false;
  }

  if (millis() - timerOne > 5000 && three) { // another five sec, and ensure the if statement goes one by one 
    sensors_event_t event; 
    dht.humidity().getEvent(&event); // dht reads humidity
    float hum = event.relative_humidity;
    char str_humd[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character
    dtostrf(event.relative_humidity, 5, 2, str_humd);  
    Serial.print("Reading from DHT: "); Serial.print(str_humd); Serial.println(" %");
    sprintf(message, "{\"humd\":\"%s\"}", str_humd);
    mqtt.publish("willa/Humd", message);  
    oled.print("Humd: "); oled.print(hum,0); oled.println(" % ");
    oled.display(); 
    timerOne = millis();
    three = false;
    one = true; // next is one
  }

  
  // grab the current state of the button.
  // we have to flip the logic because we are
  // using a pullup resistor.
  if (digitalRead(BUTTON_PIN) == LOW) {
    current = true;
  } else {
    current = false;
  }
  // return if the value hasn't changed
  if (current == last) {
    return;
  }
  
  Serial.print("button state -> ");
  Serial.println(current);

  sprintf(message, "{\"button\":\"%d\"}", current);
  mqtt.publish("willa/Button", message);  // publish the button state
  
  last = current;
}



//By subscribing to a specific channel or topic, we can listen to those topics we wish to hear.
//We place the callback in a separate tab so we can edit it easier
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; // create the buffer
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { // well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }

  root.printTo(Serial); //print out the parsed message
  Serial.println(); //give us some space on the serial monitor read out
}

// set up wifi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  Serial.println(WiFi.macAddress());  //.macAddress returns a byte array 6 bytes representing the MAC address
}                                     //5C:CF:7F:F0:B0:C1 for example

// Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("theTopic/+"); //we are subscribing to 'theTopic' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
