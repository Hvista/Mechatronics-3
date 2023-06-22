// Libraries
#include <Wire.h> // I2C connection
#include <Stepper.h>
#include <NewPing.h>


// Variables
const int stepsPerRevolution = 2048; // This should actually be 64 according to our stepper, but the value 
int function = 0; 
int max_distance = 100; // Max reading distance for echo locator
float duration, distance; // Echo locator variables
bool goodRead = false; 
char request = 'n';


// Pins and components
Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);
#define trigPin  2
#define echoPin  3
NewPing sonar(trigPin, echoPin, max_distance);


void setup() {
  Wire.begin(8);                /* join I2C bus with address 8 */
  Wire.onReceive(receiveEvent); /* register receive event */
  Wire.onRequest(requestEvent); /* register request event */
  Serial.begin(115200);           /* start serial for debug */

  // Zero stepper
  myStepper.setSpeed(10);
	myStepper.step(stepsPerRevolution/4);
  myStepper.step((-stepsPerRevolution/16)*3);
}


void loop() {
    // Function variable gets it's value from recieve event function.
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
    Serial.print(c);           /* print the character */

    if(c == 's') {
      // Serial.println("Event recieved: Step motor"); // This is commented since the arduino couldn't react to how fast the pouring happened.
      // function = 1;
    } 

    if(c == 'd') {
      Serial.println("Event recieved: Distance read cup insert");
      function = 2;
    } 

    if(c == 'r') {
      Serial.println("Event recieved: Cup removal");
      delay(500);
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
    Serial.println("Request send to ESP32: Cup insert approved");
    } else
  if(request == 'r') {
    char c = 'r';
    request = 'n';

    delay(50);
    Wire.write(c);  /*send string on request */
    Serial.println("Request send to ESP32: Cup removed approved");
  }
}

void stepUp() {
  // Rotate upwards
	myStepper.setSpeed(5);
	myStepper.step((stepsPerRevolution/24)*7);

  stepDown();
}

void stepDown() {
  // Rotate downwards slowly
	myStepper.setSpeed(3);
	myStepper.step(-stepsPerRevolution/8);
}

void dRead() {
  Serial.println("dRead called");
  int dArray[5] = {0,0,0,0,0}; // Empty array for a while loop below, that ensures the avg. between 5 readings is correct.
                               // This is implemented to avoid incorrect readings.

  while(goodRead == false) {
    for(int i = 0; i < 5; i++){
      delay(60); // Delay between each reading / pulse.

      duration = sonar.ping();
      dArray[i] = (duration / 2) * 0.0343;
      Serial.print((duration / 2) * 0.0343);
      Serial.print(" ; ");
    }

    float sum = dArray[0] + dArray[1] + dArray[2] + dArray[3] + dArray[4];
    Serial.println(sum);
    Serial.println(sum/5);
    Serial.println();

    if((sum/5 < 11) && (sum/5 > 4)) { // 11 & 4 cm is the interval for the cup to be inserted
      goodRead = true; // Exit while loop if avg. is between 11 & 4 cm.

      request = 'y'; // Send value 'y' to ESP32
      Serial.print("Good distance read");
      Serial.println();

      stepUp();
    }
    delay(1000);
  }
  goodRead = false;
}


void dRead2() { // Almost a copy of dRead1, except this reads once the distance is above something.
  Serial.println("dRead2 called");
  int dArray[5] = {0,0,0,0,0};

  while(goodRead == false) {
    for(int i = 0; i < 5; i++){
      delay(200); // Delay between each cup remove check

      duration = sonar.ping();
      dArray[i] = (duration / 2) * 0.0343;
      Serial.print((duration / 2) * 0.0343);
      Serial.print(" ; ");
    }

    float sum = dArray[0] + dArray[1] + dArray[2] + dArray[3] + dArray[4];
    Serial.println(sum);
    Serial.println(sum/5);
    Serial.println();

    if(sum/5 > 30) { // 20 is the cup distance
      goodRead = true;

      request = 'r';
      Serial.print("Cup removed");
      Serial.println();
    }
    delay(1000);
  }
  goodRead = false;
}