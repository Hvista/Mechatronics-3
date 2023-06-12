#include <Stepper.h>

const int stepsPerRevolution = 2038;
Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // Rotate CW slowly at 5 RPM
	myStepper.setSpeed(19);
	myStepper.step(stepsPerRevolution);
	delay(1000);
	
	// Rotate CCW quickly at 10 RPM
	myStepper.setSpeed(5);
	myStepper.step(-stepsPerRevolution);
	delay(1000);
}
