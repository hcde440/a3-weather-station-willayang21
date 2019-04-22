//HEAVY commenting is on . . .
//In this program we are publishing and subscribing to a MQTT server that requires a login/password
//authentication scheme. We are connecting with a unique client ID, which is required by the server.
//This unique client ID is derived from our device's MAC address, which is unique to the device, and
//thus unique to the universe.
//
//We are publishing with a generic topic ("theTopic") which you should change to ensure you are publishing
//to a known topic (eg, if everyone uses "theTopic" then everyone would be publishing over everyone else, which
//would be a mess). So, create your own topic channel.
//
//We have hardcoded the topic and the subtopics in the mqtt.publish() function, because those topics and sub
//topics are never going to change. We have subscribed to the super topic using the directory-like addressing
//system MQTT provides. We subscribe to 'theTopic/+' which means we are subscribing to 'theTopic' and every
//sub-topic that might come after the main topic. We denote this with a '+' symbol.
//
//Please change your super topic and don't use 'theTopic'.
/////

#include <ESP8266WiFi.h>    //Requisite Libraries . . .
#include "Wire.h"           //
#include <PubSubClient.h>   //
#include <ArduinoJson.h>    //


#define wifi_ssid "University of Washington"   //You have seen this before
#define wifi_password "" //

//////////
//So to clarify, we are connecting to and MQTT server
//that has a login and password authentication
//I hope you remember the user and password
//////////

#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

//////////
//We also need to publish and subscribe to topics, for this sketch are going
//to adopt a topic/subtopic addressing scheme: topic/subtopic
//////////

WiFiClient espClient;             //blah blah blah, espClient
PubSubClient mqtt(espClient);     //blah blah blah, tie PubSub (mqtt) client to WiFi client

//////////
//We need a 'truly' unique client ID for our esp8266, all client names on the server must be unique.
//Every device, app, other MQTT server, etc that connects to an MQTT server must have a unique client ID.
//This is the only way the server can keep every device separate and deal with them as individual devices/apps.
//The client ID is unique to the device.
//////////

char mac[6]; //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!

//////////
//In our loop(), we are going to create a c-string that will be our message to the MQTT server, we will
//be generous and give ourselves 200 characters in our array, if we need more, just change this number
//////////

char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array

unsigned long currentMillis, timerOne, timerTwo, timerThree; //we are using these to hold the values of our timers

/////SETUP/////
void setup() {
  Serial.begin(115200);
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback); //register the callback function
  timerOne = timerTwo = timerThree = millis();
}

/////SETUP_WIFI/////
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

/////CONNECT/RECONNECT/////Monitor the connection to MQTT server, if down, reconnect
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

/////LOOP/////
void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }

  mqtt.loop(); //this keeps the mqtt connection 'active'

  /////
  //This demo uses sprintf, which is very similar to printf,
  //read more here: https://en.wikipedia.org/wiki/Printf_format_string
  /////

  //Here we just send a regular c-string which is not formatted JSON, or json-ified.
  if (millis() - timerOne > 10000) {
    //Here we would read a sensor, perhaps, storing the value in a temporary variable
    //For this example, I will make something up . . .
    int legoBatmanIronyLevel = 200;
    sprintf(message, "{\"WillaAge\":\"%d\"}", legoBatmanIronyLevel); // %d is used for an int
    mqtt.publish("theTopic/WillaAge", message);
    timerOne = millis();
  }

  //Here we will deal with a JSON string
  if (millis() - timerTwo > 15000) {
    //Here we would read a sensor, perhaps, storing the value in a temporary variable
    //For this example, I will make something up . . .
    float temp = 78.34;
    float humidity = 56.97;

    /////
    //Unfortunately, as of this writing, sprintf (under Arduino) does not like floats  (bug!), so we can not
    //use floats in the sprintf function. Further, we need to send the temp and humidity as
    //a c-string (char array) because we want to format this message as JSON.
    //
    //We need to make these floats into c-strings via the function dtostrf(FLOAT,WIDTH,PRECISION,BUFFER).
    //To go from the float 3.14159 to a c-string "3.14" you would put in the FLOAT, the WIDTH or size of the
    //c-string (how many chars will it take up), the decimal PRECISION you want (how many decimal places, and
    //the name of a little BUFFER we can stick the new c-string into for a brief time. . .
    /////

    char str_temp[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character
    char str_humd[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character

    //take temp, format it into 5 char array with a decimal precision of 2, and store it in str_temp
    dtostrf(temp, 5, 2, str_temp);
    //ditto
    dtostrf(humidity, 5, 2, str_humd);

    /////
    //For proper JSON, we need the "name":"value" pair to be in quotes, so we use internal quotes
    //in the string, which we tell the compiler to ignore by escaping the inner quotes with the '/' character
    /////

    sprintf(message, "{\"temp\":\"%s\", \"humd\":\"%s\"}", str_temp, str_humd);
    mqtt.publish("theTopic/Willa_tempHum", message);
    timerTwo = millis();
  }

  if (millis() - timerThree > 23000) {
    boolean button = true;
    sprintf(message, "{\"magic\" : \"%d\"}", button); // %d is used for a bool as well
    mqtt.publish("theTopic/magic", message);
    timerThree = millis();
  }
}//end Loop


/////CALLBACK/////
//The callback is where we attach a listener to the incoming messages from the server.
//By subscribing to a specific channel or topic, we can listen to those topics we wish to hear.
//We place the callback in a separate tab so we can edit it easier . . . (will not appear in separate
//tab on github!)
/////

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //blah blah blah a DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }

  /////
  //We can use strcmp() -- string compare -- to check the incoming topic in case we need to do something
  //special based upon the incoming topic, like move a servo or turn on a light . . .
  //strcmp(firstString, secondString) == 0 <-- '0' means NO differences, they are ==
  /////

  if (strcmp(topic, "theTopic/WillaAge") == 0) {
    Serial.println("A message from Willa . . .");
  }

  else if (strcmp(topic, "theTopic/Willa_tempHum") == 0) {
    Serial.println("Some temp and hum info has arrived . . .");
  }

  else if (strcmp(topic, "theTopic/magic") == 0) {
    Serial.println("The magic state is being reported . . .");
  }

  root.printTo(Serial); //print out the parsed message
  Serial.println(); //give us some space on the serial monitor read out
}
