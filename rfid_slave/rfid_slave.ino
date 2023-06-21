// Libraries
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <FastLED.h>


//----- LED PIN ARRAY -----//
// i = 0; i < 6  || LOGO
// i = 13; i < 15 || BW APP
// i = 16; i < 18 || SCAN
// i = 19; i < 21 || CUP
// i = 21; i < 24 || ENJOY
//-------------------------//


// Pins and components
#define NUM_LEDS 24  // Total number of LEDs in the strip
#define LED_PIN 5    // Pin number connected to the data pin of the strip
CRGB leds[NUM_LEDS];
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);
byte keyTagUID[4] = { 0xF3, 0x15, 0xA4, 0x14 };

// Variables
int and1;
bool reset = false;  // Once process is done, reset everything
bool scanned = false;
bool cupIn = false;
bool finish = false;
unsigned long rainbowTimer = 0;
unsigned long blinkTimer = 0;
const unsigned long rainbowInterval = 60;  // Interval for rainbow shifting (in milliseconds)
const unsigned long blinkInterval = 500;   // Interval for LED blinking (in milliseconds)
bool blinkState = false;

void setup() {
  Wire.begin(9);                /* join i2c bus with address 8 */
  Wire.onReceive(receiveEvent); /* register receive event */
  Wire.onRequest(requestEvent); /* register request event */
  Serial.begin(115200);         /* start serial for debug */
  SPI.begin();                  // Initiate  SPI bus
  rfid.PCD_Init();              // init MFRC522

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);

  // BW App
  for (int i = 13; i < 15; i++) {
    leds[i] = CRGB::White;
  }
  FastLED.show();
}


void loop() {
  rainbowShift();

  if (scanned == false) {
    idReader();
    ledBlinkScan();
  } else if (cupIn == false) {
    ledBlinkCup();
  } else if (cupIn == true && finish == false) {
    cupInserted();
  } else if (finish == true) {
    finished();
  }

  if(reset == true) {
    reset = false;
    scanned = false;
    cupIn = false;
    finish = false;
  }
}


void approvedScan() {
  scanned = true;
  delay(10);  // For LED's to react, otherwise some will stay lit even though they're meant to turn off

  // BW Logo & App on approved RFID scan
  for (int i = 0; i < 15; i++) {
    leds[i] = CRGB::Green;
  }  // BW Scan on approved RFID scan
  for (int i = 16; i < 18; i++) {
    leds[i] = CRGB::Blue;
  }
  FastLED.show();
  delay(3000);
}


void rainbowShift() {
  if (millis() - rainbowTimer >= rainbowInterval) {
    static uint8_t hue = 0;  // Variable to store the hue value

    for (int8_t ledIndex = NUM_LEDS - 2; ledIndex >= 0; --ledIndex) {
      if (ledIndex >= 0 && ledIndex < 6) {
        leds[ledIndex + 1] = leds[ledIndex];
      }
    }

    leds[0] = CHSV(hue, 255, 255);  // Set the first LED to the current hue

    FastLED.show();

    // Increment hue for the next iteration
    hue += 8;

    rainbowTimer = millis();  // Reset the rainbow timer
  }
}


void ledBlinkScan() {
  if (millis() - blinkTimer >= blinkInterval) {
    if (blinkState) {
      for (int8_t ledIndex = 16; ledIndex < 18; ++ledIndex) {
        leds[ledIndex] = CRGB::Blue;  // Turn off LEDs 7-24
      }
    } else {
      for (int8_t ledIndex = 16; ledIndex < 18; ++ledIndex) {
        leds[ledIndex] = CRGB::Black;  // Turn on LEDs 7-24
      }
    }

    FastLED.show();

    blinkState = !blinkState;  // Toggle the blink state

    blinkTimer = millis();  // Reset the blink timer
  }
}


void ledBlinkCup() {
  // Turn off BW Scan and activate next step
  for (int i = 16; i < 18; i++) {
    leds[i] = CRGB::Black;
  }  // BW App
  for (int i = 13; i < 15; i++) {
    leds[i] = CRGB::White;
  }
  FastLED.show();


  if (millis() - blinkTimer >= blinkInterval) {
    if (blinkState) {
      for (int8_t ledIndex = 19; ledIndex < 21; ++ledIndex) {
        leds[ledIndex] = CRGB::OrangeRed;  // Turn off LEDs 7-24
      }
    } else {
      for (int8_t ledIndex = 19; ledIndex < 21; ++ledIndex) {
        leds[ledIndex] = CRGB::Black;  // Turn on LEDs 7-24
      }
    }

    FastLED.show();

    blinkState = !blinkState;  // Toggle the blink state

    blinkTimer = millis();  // Reset the blink timer
  }
}


void cupInserted() {
  // BW Cup
  for (int i = 19; i < 21; i++) {
    leds[i] = CRGB::OrangeRed;
  }
  FastLED.show();
}


void finished() {
  cupIn = false;
  finish = false;
}


// function that executes whenever data is received from master
void receiveEvent(int howMany) {
  while (0 < Wire.available()) {
    char c = Wire.read(); /* receive byte as a character */
    Serial.print(c);      /* print the character */

    if (c == 'd') {
      // BW App
      for (int i = 13; i < 15; i++) {
        leds[i] = CRGB::Green;
      } // BW Cup
      for (int i = 19; i < 21; i++) {
        leds[i] = CRGB::OrangeRed;
      }
      FastLED.show();
      delay(3000);


      for (int i = 13; i < 15; i++) {
        leds[i] = CRGB::White;
      }
      FastLED.show();
      cupIn = true;
    } else
    if (c == 'f') {
      // BW Cup
      for (int i = 19; i < 21; i++) {
        leds[i] = CRGB::Black;
      } // BW Enjoy
      for (int i = 21; i < 24; i++) {
        leds[i] = CRGB::Green;
      }
      FastLED.show();
      delay(3000);

      for (int i = 21; i < 24; i++) {
        leds[i] = CRGB::Black;
      }
      FastLED.show();

      finish = true;
    } else
    if (c == 'r') {
      reset = true;
    }
  }
  Serial.println(); /* to newline */
}

// function that executes whenever data is requested from master
void requestEvent() {
  if (and1 == '1') {
    Wire.write(and1);
    and1 = 0;
  }
}

void idReader() {
  if (rfid.PICC_IsNewCardPresent()) {  // new tag is available
    if (rfid.PICC_ReadCardSerial()) {  // NUID has been readed
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

      if (rfid.uid.uidByte[0] == keyTagUID[0] && rfid.uid.uidByte[1] == keyTagUID[1] && rfid.uid.uidByte[2] == keyTagUID[2] && rfid.uid.uidByte[3] == keyTagUID[3]) {
        Serial.println("Access is granted");
        and1 = '1';
        approvedScan();
      } else {
        Serial.print("Access denied, UID:");
        for (int i = 0; i < rfid.uid.size; i++) {
          Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
          Serial.print(rfid.uid.uidByte[i], HEX);
          and1 = '0';
        }
        Serial.println();
      }

      rfid.PICC_HaltA();       // halt PICC
      rfid.PCD_StopCrypto1();  // stop encryption on PCD
    }
  }
}
