#include <NewPing.h>
#include <Stepper.h>
#include <SPI.h>
#include <MFRC522.h>

// Function Pins //
#define relay 14  // Relay pin
#define sensor 27 // Flow Sensor pin
#define TRIGGER_PIN 25
#define ECHO_PIN 24
#define SS_PIN    5  // ESP32 pin GIOP5 
#define RST_PIN   12 // ESP32 pin GIOP27

// Function Variables //
int maxDistance = 10;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, maxDistance); // NewPing setup of pins and maximum distance.
float duration, distance;
bool scanned = false;
const int stepsPerRevolution = 2038;
// byte keyTagUID[4] = {0xFF, 0xFF, 0xFF, 0xFF};

MFRC522 rfid(SS_PIN, RST_PIN);
Stepper myStepper = Stepper(stepsPerRevolution, 32, 35, 33, 34);

void setup() {
  pinMode(relay, OUTPUT);         // Sets the relay as output
  digitalWrite(relay, HIGH);      // The relay starts in "ON" mode, so we send a HIGH to turn it of in the beginning
  pinMode(sensor, INPUT_PULLUP);  // Sets the sensor as output

  Serial.begin(115200); 

  pinMode(TRIGGER_PIN, OUTPUT); // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT); // Sets the echoPin as an Input

  pulseCount = 0;
  rateOfFlow = 0.0;
  flowMilli = 0;
  totalMilli = 0;
  previousTime = 0;

  attachInterrupt(digitalPinToInterrupt(sensor), pulseCounter, FALLING);

  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
}

void appScan() {
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      Serial.print("RFID/NFC Tag Type: ");
      Serial.println(rfid.PICC_GetTypeName(piccType));

      // print UID in Serial Monitor in the hex format
      Serial.print("UID:");
      for (int i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
      }
      Serial.println();

      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
}

void checkForCup() {
  duration = sonar.ping();
  distance = (duration / 2) * 0.0343;
  
  Serial.print("Distance = ");
  Serial.print(distance); // Distance will be 0 when out of set max range.
  Serial.println(" cm");
}

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

void relaySlider() {
  // The function controls what percentage of the duration for a whole beer tap that the relay should be turned on based on the slider input value
  int sliderVal = 2000  // Converts the recieved payload into an integer and converts it to the duration of which the
    for (int i = 0; i < sliderVal; i++) {
    digitalWrite(relay, LOW);   // Relay turns on
    delay(sliderVal);           // Relay is on for the duration received from the payload
    digitalWrite(relay, HIGH);  // Relay is turned off
    and1 = 0;
  }
}

void stepTest() {
  myStepper.setSpeed(10);
	myStepper.step(-stepsPerRevolution);
}

void loop() {
  delay(100);
  appScan();
  flowSensor();
  checkForCup();
  relaySlider();
}
