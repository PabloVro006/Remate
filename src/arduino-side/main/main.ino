// Include config header file
#include "config.h"

// SETUP
void setup() {
  // SERIAL INITIALIZATION
  delay(500);
  Serial.begin(115200);
  delay(serialDelay);

  // HALL INITIALIZATION
  // Setting this to input for reading the hall's output
  pinMode(HALL_DISK, INPUT);
  pinMode(HALL_CROSS, INPUT);

  // TRANSISTOR INITIALIZATION
  pinMode(PADDLE_NPN, OUTPUT);
  // Setting the disk's and cross's transistors
  for (int i = 0; i < 2; i++) {
    pinMode(motorData[i].COUNTER_PIN, OUTPUT);
    pinMode(motorData[i].CLOCK_PIN, OUTPUT);
    digitalWrite(motorData[i].COUNTER_PIN, LOW);
    digitalWrite(motorData[i].CLOCK_PIN, LOW);
  }
  // Make the paddle move
  paddleMotorStruct.power = 1;
  paddleMotorStruct.going = 1;

  // CALIBRATION
  rotateMotor(1, CLOCKWISE, 1); // Cross calibration
  resetOffset(1, COUNTER_CLOCKWISE, 150);
}

// LOOP
void loop() {
  controlPaddleMotorGoing(&paddleMotorStruct);

  // Get trash only if Remate is not throwing (motors aren't moving)
  if(!isThrowing){
    trash = getTrashFromPi();
    // Condition for when the Rpi4 has sent something
    if(trash != TRASH_NONE){
      // Stops the paddle
      paddleMotorStruct.power = 0;
      controlPaddleMotorPower(&paddleMotorStruct);
      if(trash == TRASH_INCOMING){
        // Wait max 5 seconds for the defined trash
        ul startMillis = millis();
        while(trash == TRASH_INCOMING || trash == TRASH_NONE){
          ul currentMillisForTrashIncoming = millis();
          trash = getTrashFromPi();
          delay(serialDelay);
          // Checking if the 5 seconds max has passed
          if(currentMillisForTrashIncoming - startMillis >= trashIncomingTimeout){
            // Resets all the variable and restarts the loop()
            trash = TRASH_NONE;
            paddleMotorStruct.power = 1;
            return;
          }
        };
      }
      isThrowing = true;  // This is useful for stopping the data reception from the Rpi4
      if(trash == TRASH_METAL || trash == TRASH_PLASTIC){
        throwPOM(trash);
      } else {
        throwPaper();
      }
      // Send feedbakc to Rpi4 and start moving the paddle again
      sendFeedbackToPi(feedbackOk);
      paddleMotorStruct.power = 1;
      paddleMotorStruct.going = 0;
    }
  }
  // Call the function for moving the paddle based on the parameters that as been changed
  controlPaddleMotorPower(&paddleMotorStruct);
}