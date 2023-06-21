#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>

// Function Pins //
#define relay 14   // Relay pin
#define sensor 27  // Flow Sensor pin

// Water flow sensor function variables //
unsigned long currentMillis;    // Variable used to log current time
unsigned long previousMillis;   // Variable used to save previous logged time
int flowInterval = 1000;        // Interval used for the sensor
float calibrationFactor = 4.5;  // Variable used for deviation in sensor readings
volatile byte pulseCount;       // Variable for pulse readings in flow sensor
byte pulse1Sec = 0;
int flowRate;                      // Flow rate chosen from the user interview
unsigned int flowMilliLitres = 1;  // Flow rate logged from water flow sensor
unsigned long totalMilliLitres;  // Total logged millillitres read from water flow sensor
unsigned long totalLoggedMillilitres; // Total logged millilitres in a saved value

// UI input variables
int totalKeg = 1;                // Total size of kegged registered from user
int sliderVal;       // Value sent from slider
int slideGate;       // Variable used to create and and-gate so only sliderVal can pass through the rest of the code
int relayGate;       // Variable used to create and and-gate so only a preset value can pass through the rest of the code
int saldo;           // How much money the user inserts into the UI
int newSaldo;        // The updated money amount based on the purchase chosen
int beerPrice;       // Input price of 1 beer
int glassSize;       // Input size of 1 glass of beer
const int tol = 10;  // Tolerance from relay close till it actually stops pouring


// Boolean Variables //
bool inProcess = false;     // Check if beer pouring is in progress
bool distanceRead = false;  //  Check if HC-SR04 Module is reading
bool halfFull = false;      // Check if
bool cupFull = false;       // Check if cup is full based on input cup size


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

  if (topic == "s204719@student.dtu.dk/glassSize") {  // This topic receives the input from the UI buttons
    payload = "";
    for (int i = 0; i < length; i++) {
      payload += (char)byteArrayPayload[i];
    }
    Serial.println(payload);  // Prints the payload
    glassSize = payload.toInt();
  }
  if (topic == "s204719@student.dtu.dk/kegSize") {  // This topic receives the input from the UI buttons
    payload = "";
    for (int i = 0; i < length; i++) {
      payload += (char)byteArrayPayload[i];
    }
    Serial.println(payload);  // Prints the payload
    totalKeg = payload.toInt();
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
      client.subscribe("s204719@student.dtu.dk/saldo");
      client.subscribe("s204719@student.dtu.dk/price");
      client.subscribe("s204719@student.dtu.dk/glassSize");
      client.subscribe("s204719@student.dtu.dk/kegSize");
    } else {  // If the connection fails, the loop will try again after 5 seconds until a connection has been established
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

  // All water flow sensor variables are set to 0 during setup //
  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(sensor), pulseCounter, FALLING);

  Wire.begin(21, 22);  // Sets up connection from ESP32 to Arduino Unos through SDA (Serial Data) and SCL (Serial Clock) pins
}
// Beer pouring function //
void pouringFunctions() {
  distance1();        // We first check if a cup is present in front of the HC-SR04 module, thereby it also being under the faucet
  relayFunc();        // Thereafter, the relay function is called to control the pouring of the beverage
  distance2();        // We then check if the cup is removed before pouring another beverage
  inProcess = false;  // inProcess boolean is set to false to alow further pouring (reset)
}
// Cup sensor registration function //
void distance1() {            // Prevents the valve opening before a cup has been inserted
  Wire.beginTransmission(8);  // begin with device address 8
  Wire.write("d");            // Call dRead function from arduino with adress 8
  Serial.print("Transmission sent");
  Wire.endTransmission();  // stop transmitting

  while (distanceRead == false) {
    Wire.requestFrom(8, 1);  // request & read data of size 1 from worker
    while (Wire.available()) {
      char c = Wire.read();  // received data is stored in the variable "c"

      if (c == 'y') {
        delay(1000);  // Adjust to time it takes for step motor to tilt cup
        distanceRead = true;
      }
    }
  }
  distanceRead = false;  // Reset
}


void distance2() {            // Prevents a new process to begin until cup has been removed
  Wire.beginTransmission(8);  // begin with device address 8
  Wire.write("r");            // Call dRead function from arduino with adress 8
  Serial.print("Transmission sent");
  Wire.endTransmission(); /* stop transmitting */

  while (distanceRead == false) {
    Wire.requestFrom(8, 1);  // request & read data of size 1 from worker
    while (Wire.available()) {
      char c = Wire.read();

      if (c == 'r') {
        distanceRead = true;
      }
    }
  }
  distanceRead = false;  // Reset
}


// Flow sensor control //
void flowSensor() {
  currentMillis = millis();
  if (currentMillis - previousMillis > flowInterval) {

    pulse1Sec = pulseCount;
    pulseCount = 0;

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
    totalLoggedMillilitres += flowMilliLitres;

    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");  // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(totalMilliLitres / 1000);
    Serial.println("L");
  }
}


// Function control for beer serving //
void relayFunc() {
  digitalWrite(relay, LOW);  // Open valve and start pouring

  while (cupFull == false) {  // While cup isn't full
    flowSensor();             // Flow sensor function is called

    if (totalMilliLitres >= glassSize - tol) {  // If total read millilitres if greater than input glass size
      totalMilliLitres = 0;                     // Reset total read millilitres
      cupFull = true;                           // Register cup is now full
    }

    if ((totalMilliLitres >= glassSize / 2) && (halfFull == false)) {
      halfFull = true;

      Wire.beginTransmission(8); // Begin sending data to device with address 8 
      Wire.write("s");           // Sends string
      Serial.print("Transmission sent regarding stepDown");
      Wire.endTransmission(); // stop transmitting 
    }
    delay(50);  // How often it checks the current amount poured
  }
  Wire.beginTransmission(8); /* begin with device address 8 */
  Wire.write("r");           /* sends hello string */
  Serial.print("Transmission sent regarding cup removal");
  Wire.endTransmission(); /* stop transmitting */

  digitalWrite(relay, HIGH);  // Close valve once glass is full
  cupFull = false;            // Reset
  halfFull = false;
}


// Relay Control //
void relayControl() {
  // The function controls what percentage of the duration for a whole beer tap that the relay should be turned on
  if (payload == "smagsprøve") {
    newSaldo = saldo - beerPrice;

  } else if (payload == "halv") {
    newSaldo = saldo - beerPrice * 3;
    for (int i = 0; i < 3; i++) {
    }
  } else if (payload == "hel") {
    newSaldo = saldo - beerPrice * 5;
    for (int i = 0; i < 5; i++) {
    }
  }
  client.publish("s204719@student.dtu.dk/saldo", String(newSaldo).c_str());
}


// Relay Slider //
void relaySlider() {
  // The function controls what percentage of the duration for a whole beer tap that the relay should be turned on based on the slider input value
  newSaldo = saldo - beerPrice * sliderVal;
  client.publish("s204719@student.dtu.dk/saldo", String(newSaldo).c_str());
  for (int i = 0; i < sliderVal; i++) {
    delay(200);
  }
}


// Main Loop //
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Wire.requestFrom(9, 1); /* request & read data of size 13 from slave */
  // while (Wire.available()) {
  //   char c = Wire.read();
  //   if (c == '1') {
  //     and1 = 1;
  //   } else {
  //     and1 = 0;
  //   }
  // }
  if (inProcess == false) {
    Wire.requestFrom(9, 1); /* request & read data of size 13 from slave */
    while (Wire.available()) {
      char c = Wire.read();

      if (c == '1') {
        inProcess = true;
        Serial.println("Process began");

        if (relayGate == 1) {
          relayControl();
          relayGate = 0;
        } else if (slideGate == 1) {
          relaySlider();
          slideGate = 0;
        }
      }
    }
  }
  if (totalLoggedMillilitres > (totalKeg - 0.2) * 1000){
    client.publish("s204719@student.dtu.dk/kegSize", "empty");
    totalLoggedMillilitres = 0;
  }
}