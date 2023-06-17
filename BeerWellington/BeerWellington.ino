#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>

// Function Pins //
#define relay 14   // Relay pin
#define sensor 27  // Flow Sensor pin

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
int fullGlassTime = 1;
int totalKeg = 1;
const int flowThreshold = 50;
int and1;
int sliderVal;
int slideGate;
int relayGate;
int saldo;
int newSaldo;
int beerPrice;

// WiFi Variables //
const char *ssid = "WiFimodem-272D";  // Wifi name
const char *password = "qtzqgzqwtz";  // Wifi pass

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
    relayGate = 1;
    slideGate = 0;
  }
  if (topic == "s204719@student.dtu.dk/beerSlider") {  // This topic reveives the input from the UI slider
    payload = "";
    for (int i = 0; i < length; i++) {
      payload += (char)byteArrayPayload[i];
    }
    Serial.println(payload);      // Prints the payload
    sliderVal = payload.toInt();  // Converts the recieved payload into an integer
    relayGate = 0;
    slideGate = 1;
  }
  if (topic == "s204719@student.dtu.dk/saldo") {  // This topic reveives the input from the UI slider
    payload = "";
    for (int i = 0; i < length; i++) {
      payload += (char)byteArrayPayload[i];
    }
    Serial.println(payload);  // Prints the payload
    saldo = payload.toInt();  // Converts the recieved payload into an integer
  }
  if (topic == "s204719@student.dtu.dk/price") {  // This topic receives the input from the UI buttons
    payload = "";
    for (int i = 0; i < length; i++) {
      payload += (char)byteArrayPayload[i];
    }
    Serial.println(payload);  // Prints the payload
    beerPrice = payload.toInt();
  }
  // if (topic == "s204719@student.dtu.dk/glassSize") {  // This topic receives the input from the UI buttons
  //   payload = "";
  //   for (int i = 0; i < length; i++) {
  //     payload += (char)byteArrayPayload[i];
  //   }
  //   Serial.println(payload);  // Prints the payload
  //   fullGlass = payload.toInt();
  // }
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
      client.subscribe("s204719@student.dtu.dk/saldo");
      client.subscribe("s204719@student.dtu.dk/price");
      client.subscribe("s204719@student.dtu.dk/beers");
    } else {  // Hvis forbindelsen fejler køres loopet igen efter 5 sekunder indtil forbindelse er oprettet
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
  pinMode(relay, OUTPUT);         // Sets the relay as output
  digitalWrite(relay, HIGH);      // The relay starts in "ON" mode, so we send a HIGH to turn it of in the beginning
  pinMode(sensor, INPUT_PULLUP);  // Sets the sensor as output

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

  Wire.begin(21, 22);
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
    delay((fullGlassTime)*1000);
    digitalWrite(relay, HIGH);
    Serial.println("den lille");
  } else if (payload == "halv") {  //50% of the whole duration
    for (int i = 0; i < 3; i++) {
      digitalWrite(relay, LOW);
      delay((fullGlassTime)*1000);
      digitalWrite(relay, HIGH);
      Serial.println("den halve");
    }
  } else if (payload == "hel") {  // 100% of the whole duration
    for (int i = 0; i < 5; i++) {
      digitalWrite(relay, LOW);
      delay((fullGlassTime)*1000);
      digitalWrite(relay, HIGH);
      Serial.println("den hele");
    }
  } else {  // In the standard state, the relay is turned off
    digitalWrite(relay, HIGH);
    }
}
// Relay Control //
// void relayControl() {
//   // The function controls what percentage of the duration for a whole beer tap that the relay should be turned on
//   if (and1 = 1) {
//     if (payload == "smagsprøve") {  // 10% of the whole duration
//       digitalWrite(relay, LOW);
//       delay((fullGlassTime)*1000);
//       digitalWrite(relay, HIGH);
//       and1 = 0;
//       Serial.print("den lille");
//     } else if (payload == "halv") {  //50% of the whole duration
//       for (int i = 0; i < 3; i++) {
//         digitalWrite(relay, LOW);
//         delay((fullGlassTime)*1000);
//         digitalWrite(relay, HIGH);
//         and1 = 0;
//         Serial.print("den halve");
//       }
//     } else if (payload == "hel") {  // 100% of the whole duration
//       for (int i = 0; i < 5; i++) {
//         digitalWrite(relay, LOW);
//         delay((fullGlassTime)*1000);
//         digitalWrite(relay, HIGH);
//         and1 = 0;
//         Serial.print("den hele");
//       }
//     } else {  // In the standard state, the relay is turned off
//       digitalWrite(relay, HIGH);
//       and1 = 0;
//     }
//   }
// }
// Relay Slider //
void relaySlider() {
  // The function controls what percentage of the duration for a whole beer tap that the relay should be turned on based on the slider input value
  newSaldo = saldo - beerPrice * sliderVal;
  client.publish("s204719@student.dtu.dk/saldo", String(newSaldo).c_str());
  for (int i = 0; i < sliderVal; i++) {
    // Serial.print(sliderVal); Serial.print(" test"); Serial.println();
    int newSliderVal = sliderVal - i;
    client.publish("s204719@student.dtu.dk/beers", String(newSliderVal).c_str());
    delay(500);
    
      if (and1 == 1) {
      int sliderVal = payload.toInt();  // Converts the recieved payload into an integer and converts it to the duration of which the
      for (int i = 0; i < sliderVal; i++) {
        digitalWrite(relay, LOW);   // Relay turns on
        delay(sliderVal * 1000);    // Relay is on for the duration received from the payload
        digitalWrite(relay, HIGH);  // Relay is turned off
        and1 = 0;
        Serial.print("test");
      }

    }
    // digitalWrite(relay, LOW);   // Relay turns on
    // delay(sliderVal * 1000);    // Relay is on for the duration received from the payload
    // digitalWrite(relay, HIGH);  // Relay is turned off
  }
}
// Main Loop //
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  flowSensor();

  if (and1 == 1 && slideGate == 1) {
    relaySlider();  // Calls the relay slider function
    slideGate = 0;
  }
  if (and1 == 1 && relayGate == 1) {
    relayControl();
    relayGate = 0;
  }

  Wire.requestFrom(8, 1); /* request & read data of size 13 from slave */
  while (Wire.available()) {
    char c = Wire.read();
    if (c == '1') {
      and1 = 1;
    } else {
      and1 = 0;
    }
  }
}
