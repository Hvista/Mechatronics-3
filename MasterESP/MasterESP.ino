// Libraries
#include <Wire.h>


// Variables
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
bool cupFull = false;
int glassSize = 500; // Should be configurable in NodeRed (nice to have)
const int tol = 10; // Tolerance from relay close till it actually stops pouring
bool halfFull = false;
bool inProcess = false;

// Pins and components
#define relay 14  // Relay pin
#define sensor 27


void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}


void setup() {
  Serial.begin(115200); /* begin serial for debug */
  Wire.begin(21, 22); /* join i2c bus with SDA=D1 and SCL=D2 of NodeMCU */

  pinMode(relay, OUTPUT);     // Sets the relay as output
  digitalWrite(relay, HIGH);  // The relay starts in "ON" mode, so we send a HIGH to turn it of in the beginning
  pinMode(sensor, INPUT_PULLUP);

  Wire.beginTransmission(8); /* begin with device address 8 */
  Wire.write("d");  /* sends hello string */
  Serial.print("Transmission sent");
  Wire.endTransmission();    /* stop transmitting */

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(sensor), pulseCounter, FALLING);
}


void flowSensor() {
  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    
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
    
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");       // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(totalMilliLitres / 1000);
    Serial.println("L");
  }
}


void relayFunc() {
  digitalWrite(relay, LOW); // Open valve and start pouring

  while(cupFull == false) {
    flowSensor();

    if(totalMilliLitres >= glassSize - tol) {
      totalMilliLitres = 0;
      cupFull = true;
    }

    if((totalMilliLitres >= glassSize/2) && (halfFull == false)) {
      halfFull = true;

      Wire.beginTransmission(8); /* begin with device address 8 */
      Wire.write("s");  /* sends hello string */
      Serial.print("Transmission sent regarding stepDown");
      Wire.endTransmission();    /* stop transmitting */
    }
    delay(50); // How often it checks the current amount poured
  }
  Wire.beginTransmission(8); /* begin with device address 8 */
  Wire.write("r");  /* sends hello string */
  Serial.print("Transmission sent regarding cup removal");
  Wire.endTransmission();    /* stop transmitting */

  digitalWrite(relay, HIGH); // Close valve once glass is full
  cupFull = false; // Reset
  halfFull = false;
}


void loop() {
//  Wire.beginTransmission(8); /* begin with device address 8 */
//  Wire.write("d");  /* sends hello string */
//  Serial.print("Transmission sent");
//  Wire.endTransmission();    /* stop transmitting */

  if(inProcess == false) {
    Wire.requestFrom(9, 1); /* request & read data of size 13 from slave */
    while(Wire.available()){
      char c = Wire.read();

      if(c == '1') {
        inProcess = true;

        Serial.println();
        Serial.print(c);

        Wire.beginTransmission(8); /* begin with device address 8 */
        Wire.write("d");  // Call dRead function from arduino with adress 8
        Serial.print("Transmission sent");
        Wire.endTransmission();    /* stop transmitting */
      }
    }
  }

  Wire.requestFrom(8, 1); /* request & read data of size 13 from slave */
  while(Wire.available()){
    char c = Wire.read();

    if(c == 'y') {
      delay(1000); // Adjust to time it takes for step motor to tilt cup
      relayFunc();
    }

    if(c == 'r') {
      inProcess = false;
      Serial.print("Cup removed! Shit works :O !");
    }
  }

  delay(1000);
}