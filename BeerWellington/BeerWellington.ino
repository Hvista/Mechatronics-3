#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <SPI.h> 
#include <MFRC522.h>

// Function Pins //
#define relay 14  // Relay pin number
#define SS_PIN 5
#define RST_PIN 27

MFRC522 rfid(SS_PIN, RST_PIN);

// Function Variables //
int rateOfFlow;  // Flow rate chosen from the user interview
byte authorizedUID[4] = {0xF3, 0x15, 0xA4, 0x14}; // Authorized ID 
unsigned long previousTime;
unsigned long currentTime;


// WiFi Variables //
const char *ssid = "Stampe";  // Wifi name
const char *password = "whit3field";  // Wifi pass

// MQTT Broker Variables //
const char *mqtt_server = "maqiatto.com";          // MQTT server name
const int mqtt_port = 1883;                        // MQTT port
const char *mqtt_user = "s204719@student.dtu.dk";  // MQTT user
const char *mqtt_pass = "Mekatronic";              // MQTT pass

// Callback Function Variables //
String payload;
void callback(char *byteArraytopic, byte *byteArrayPayload, unsigned int length);
WiFiClient espClient;
PubSubClient client(mqtt_server, mqtt_port, callback, espClient);

// Connect to Wifi //
void setup_wifi() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);  // attempt to connect

  while (WiFi.status() != WL_CONNECTED) {  //wait to connect
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("//");
}
// Callback Function //
void callback(char *byteArraytopic, byte *byteArrayPayload, unsigned int length) {
  // Konverterer indkomne besked (topic) til en string:
  String topic;
  topic = String(byteArraytopic);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");
  // Konverterer den indkomne besked (payload) fra en array til en string:

  if (topic == "s204719@student.dtu.dk/flowRate") {  // This topic receives the flow rate from the user input
    payload = "";                                    // Resets the payload
    for (int i = 0; i < length; i++) {
      payload += (char)byteArrayPayload[i];
    }
    Serial.println(payload);       // Prints the payload
    rateOfFlow = payload.toInt();  // the recieved flow rate is converter
  }

  if (topic == "s204719@student.dtu.dk/beerwell") {  // This topic receives the input from the UI buttons
    payload = "";
    for (int i = 0; i < length; i++) {
      payload += (char)byteArrayPayload[i];
    }
    Serial.println(payload);  // Prints the payload
    relayControl();           // Calls the function for the relay control
  }

  if (topic == "s204719@student.dtu.dk/beerSlider") {  // This topic reveives the input from the UI slider
    payload = "";
    for (int i = 0; i < length; i++) {
      payload += (char)byteArrayPayload[i];
    }
    Serial.println(payload);  // Prints the payload
    relaySlider();            // Calls the relay slider function
  }
}
// MQTT Connection //
void reconnect() {
  // Fortsætter til forbindelsen er oprettet
  while (!client.connected()) {
    Serial.println("Establishing MQTT Connection");
    Serial.println(".");
    if (client.connect("GroupNamexMCU", mqtt_user, mqtt_pass)) {  // Forbinder til klient med mqtt bruger og password
      Serial.println("Connected to MQTT server");
      // MQTT Subscriptions //
      client.subscribe("s204719@student.dtu.dk/beerwell");
      client.subscribe("s204719@student.dtu.dk/beerSlider");
      client.subscribe("s204719@student.dtu.dk/flowRate");
    } else { // Hvis forbindelsen fejler køres loopet igen efter 5 sekunder indtil forbindelse er oprettet
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
// Setup //
void setup() {
  pinMode(relay, OUTPUT);     // Sets the relay as output
  digitalWrite(relay, HIGH);  // The relay starts in "ON" mode, so we send a HIGH to turn it of in the beginning

  Serial.begin(115200);                      // Baud rate
  SPI.begin();
  rfid.PCD_Init();
  setup_wifi();                              // Setup wifi function
  client.setServer(mqtt_server, mqtt_port);  // Connects to MQTT broker
  client.setCallback(callback);              // Ingangsætter den definerede callback funktion hver gang der er en ny besked på den subscribede "cmd"- topic
}
// Relay Control //
void relayControl() {
  // The function controls what percentage of the duration for a whole beer tap that the relay should be turned on
  if (payload == "smagsprøve") {  // 10% of the whole duration
    digitalWrite(relay, LOW);
    delay((rateOfFlow * 0.1) * 1000);
    digitalWrite(relay, HIGH);
    
  } else if (payload == "halv") {  //50% of the whole duration
    digitalWrite(relay, LOW);
    delay((rateOfFlow * 0.5) * 1000);
    digitalWrite(relay, HIGH);
   
  } else if (payload == "hel") {  // 100% of the whole duration
    digitalWrite(relay, LOW);
    delay(rateOfFlow * 1000);
    digitalWrite(relay, HIGH);
    
  } else {  // In the standard state, the relay is turned off
    digitalWrite(relay, HIGH);
  }
}
// Relay Slider //
void relaySlider() {
  // The function controls what percentage of the duration for a whole beer tap that the relay should be turned on based on the slider input value
  int sliderVal = (payload.toInt() * (rateOfFlow / 10)) * 1000;  // Converts the recieved payload into an integer and converts it to the duration of which the
  digitalWrite(relay, LOW);                                      // Relay turns on
  delay(sliderVal);                                              // Relay is on for the duration received from the payload
  digitalWrite(relay, HIGH);                                     // Relay is turned off
}
// Main Loop //
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
