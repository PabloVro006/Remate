// Include config header file
#include "config.h"

// SETUP
void setup() {
  // SERIAL INITIALIZATION
  delay(serialDelay);
  Serial.begin(115200);
  delay(serialDelay);

  // PINS INITIALIZATION
  pinMode(PADDLE_NPN, OUTPUT);
  // Setting the disk's and cross's pins
  for (int i = 0; i < 2; i++) {
    pinMode(motorData[i].COUNTER_PIN, OUTPUT);
    pinMode(motorData[i].CLOCK_PIN, OUTPUT);
    pinMode(motorData[i].HALL, INPUT);  // Setting this to input for reading the hall's output
    digitalWrite(motorData[i].COUNTER_PIN, LOW);
    digitalWrite(motorData[i].CLOCK_PIN, LOW);
  }

  // CALIBRATION
  rotateMotor(CROSS, CLOCKWISE, 1); // Cross calibration
  resetMotorOffset(CROSS, COUNTER_CLOCKWISE, 150);
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
        // In order to break the while or a trash is detected or 5 seconds passes
        while(trash == TRASH_INCOMING || trash == TRASH_NONE){
          ul currentMillisForTrashIncoming = millis();
          trash = getTrashFromPi();  // While waiting keeps to update the trash variable
          delay(serialDelay);
          // Checking if the 5 seconds max has passed
          if(currentMillisForTrashIncoming - startMillis >= trashIncomingTimeout){
            // Resets all the variable and restarts the loop()
            trash = TRASH_NONE;  // Re-setting the trash variable
            paddleMotorStruct.power = 1;  // Make the paddle move again
            return;
          }
        };
      }
      isThrowing = true;  // This is useful for stopping the data reception from the Rpi4
      if(trash == TRASH_METAL || trash == TRASH_PLASTIC){
        throwPOM(trash);
      } else {  // At this point trash must be TRASH_PAPER
        throwPaper();
      }
      // Send feedback to Rpi4 and start moving the paddle again
      sendFeedbackToPi(feedbackOk);
      paddleMotorStruct = {1, 0};  // Power = 1, going = 0
    }
  }
  // Call the function for moving the paddle based on the parameters that as been changed
  controlPaddleMotorPower(&paddleMotorStruct);
}