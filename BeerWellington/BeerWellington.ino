#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <SPI.h> 

// Function Pins //
#define relay 14  // Relay pin
#define sensor 27 // Flow Sensor pin

// Function Variables //
unsigned long currentTime;
unsigned long previousTime;
int flowInterval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
int rateOfFlow;  // Flow rate chosen from the user interview
unsigned int flowMilli = 1;
unsigned long totalMilli;
int fullGlass = 1;
int fullGlassTime;
int totalKeg;

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
  if (topic == "s204719@student.dtu.dk/kegSize") {  // This topic receives the input from the UI buttons
    payload = "";
    for (int i = 0; i < length; i++) {
      payload += (char)byteArrayPayload[i];
    }
    Serial.println(payload);  // Prints the payload
    totalKeg = payload.toInt();
  }
  if (topic == "s204719@student.dtu.dk/glassSize") {  // This topic receives the input from the UI buttons
    payload = "";
    for (int i = 0; i < length; i++) {
      payload += (char)byteArrayPayload[i];
    }
    Serial.println(payload);  // Prints the payload
    fullGlass = payload.toInt();
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
    } else { // Hvis forbindelsen fejler køres loopet igen efter 5 sekunder indtil forbindelse er oprettet
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
// Function that make the interruption possible //
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}
// Setup //
void setup() {
  pinMode(relay, OUTPUT);     // Sets the relay as output
  digitalWrite(relay, HIGH);  // The relay starts in "ON" mode, so we send a HIGH to turn it of in the beginning
  pinMode(sensor, INPUT_PULLUP); // Sets the sensor as output

  Serial.begin(115200);                      // Baud rate
  setup_wifi();                              // Setup wifi function
  client.setServer(mqtt_server, mqtt_port);  // Connects to MQTT broker
  client.setCallback(callback);              // Ingangsætter den definerede callback funktion hver gang der er en ny besked på den subscribede "cmd"- topic

  pulseCount = 0;
  rateOfFlow = 0.0;
  flowMilli = 0;
  totalMilli = 0;
  previousTime = 0;

  attachInterrupt(digitalPinToInterrupt(sensor), pulseCounter, FALLING);
}
// Flow Sensor Control //
void flowSensor() {
  currentTime = millis();
  if (currentTime - previousTime > flowInterval) {
    pulse1Sec = pulseCount;
    pulseCount = 0;

    rateOfFlow = ((1000.0 / (millis() - previousTime)) * pulse1Sec) / calibrationFactor;
    previousTime = currentTime;

    flowMilli = (rateOfFlow / 60) * 1000;

    fullGlassTime = fullGlass / flowMilli;

    totalMilli += flowMilli;
  }
}
// Relay Control //
void relayControl() {
  // The function controls what percentage of the duration for a whole beer tap that the relay should be turned on
  if (payload == "smagsprøve") {  // 10% of the whole duration
    digitalWrite(relay, LOW);
    delay((fullGlassTime * 0.1) * 1000);
    digitalWrite(relay, HIGH);
    
  } else if (payload == "halv") {  //50% of the whole duration
    digitalWrite(relay, LOW);
    delay((fullGlassTime * 0.5) * 1000);
    digitalWrite(relay, HIGH);
   
  } else if (payload == "hel") {  // 100% of the whole duration
    digitalWrite(relay, LOW);
    delay(fullGlassTime);
    digitalWrite(relay, HIGH);
  } else {  // In the standard state, the relay is turned off
    digitalWrite(relay, HIGH);
  }
}
// Relay Slider //
void relaySlider() {
  // The function controls what percentage of the duration for a whole beer tap that the relay should be turned on based on the slider input value
  int sliderVal = (payload.toInt() * (fullGlassTime / 10)) * 1000;  // Converts the recieved payload into an integer and converts it to the duration of which the
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

  flowSensor();
}
