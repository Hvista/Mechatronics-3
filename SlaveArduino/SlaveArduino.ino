// Libraries
#include <Wire.h>
#include <Stepper.h>
#include <NewPing.h>


// Variables
const int stepsPerRevolution = 2038;
int function = 0;
int max_distance = 100;
float duration, distance;
bool goodRead = false;
char request = 'n';


// Pins and components
Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);
#define trigPin  2
#define echoPin  3
NewPing sonar(trigPin, echoPin, max_distance);


void setup() {
  Wire.begin(8);                /* join i2c bus with address 8 */
  Wire.onReceive(receiveEvent); /* register receive event */
  Wire.onRequest(requestEvent); /* register request event */
  Serial.begin(115200);           /* start serial for debug */
}


void loop() {
    if (function == 1) {
      Serial.print("Call stepDown motor function");
      stepDown();
      function = 0;
    } else 
    if (function == 2) {
      Serial.print("Call distance read function");
      dRead();
      function = 0;
    } else 
    if (function == 3) {
      Serial.print("Call distance remove (dRead2) function");
      dRead2();
      function = 0;
    }
}


// function that executes whenever data is received from master
void receiveEvent(int howMany) {
 while (0 <Wire.available()) {
    char c = Wire.read();      /* receive byte as a character */
    // Serial.print(c);           /* print the character */

    if(c == 's') {
      function = 1;
    } 

    if(c == 'd') {
      function = 2;
    } 

    if(c == 'r') {
      function = 3;
    }
  }
 Serial.println();             /* to newline */
}

// function that executes whenever data is requested from master
void requestEvent() {
  if(request == 'y') {
    char c = 'y';
    request = 'n';

    Wire.write(c);  /*send string on request */
    Serial.print("Request send to ESP32");
    } else
  if(request == 'r') {
    char c = 'r';
    request = 'n';

    Wire.write(c);  /*send string on request */
    Serial.print("Request send to ESP32");
  }
}

void stepUp() {
  delay(1000);
  // Rotate upwards
	myStepper.setSpeed(5);
	myStepper.step(-stepsPerRevolution/8);
}

void stepDown() {
  // Rotate downwards slowly
	myStepper.setSpeed(3);
	myStepper.step(stepsPerRevolution/8);
}

void dRead() {
  int dArray[5] = {0,0,0,0,0};

  while(goodRead == false) {
    for(int i = 0; i < 5; i++){
      delay(60);

      duration = sonar.ping();
      dArray[i] = (duration / 2) * 0.0343;
      Serial.print((duration / 2) * 0.0343);
      Serial.print(" ; ");
    }

    float sum = dArray[0] + dArray[1] + dArray[2] + dArray[3] + dArray[4];
    Serial.println(sum);
    Serial.println(sum/5);
    Serial.println();

    if((sum/5 < 30) && (sum/5 > 20)) { // 30 & 20 are the tolerance for cup distance
      goodRead = true;

      request = 'y';
      Serial.print("Good distance read");
      Serial.println();

      stepUp();
    }
    delay(1000);
  }
  goodRead = false;
}


void dRead2() {
  int dArray[5] = {0,0,0,0,0};

  while(goodRead == false) {
    for(int i = 0; i < 5; i++){
      delay(60);

      duration = sonar.ping();
      dArray[i] = (duration / 2) * 0.0343;
      Serial.print((duration / 2) * 0.0343);
      Serial.print(" ; ");
    }

    float sum = dArray[0] + dArray[1] + dArray[2] + dArray[3] + dArray[4];
    Serial.println(sum);
    Serial.println(sum/5);
    Serial.println();

    if(sum/5 > 20) { // 20 is the cup distance
      goodRead = true;

      request = 'r';
      Serial.print("Cup removed");
      Serial.println();
    }
    delay(1000);
  }
  goodRead = false;
}