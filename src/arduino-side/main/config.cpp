// Include config header file
#include "config.h"

// DEFINE FUNCTIONS
// Reads the output values of the hall passed in the parameter
bool hallCheck(int hall) {
  int reading = analogRead(hall);  // Read the output of the hall passed and stores it in the 'reading' variable
  return (reading > hallThresholdLow && reading < hallThresholdHigh);  // Check if a magnet is detected, if so returns false
}

/*
The parameter is a list containg all the motor's indexes that must be turned off.
The last value of the list is a 0xFF flag, indicating the end for the loop.
*/
void turnMotorsOff(const int motorIndexes[]){
  // Setting to LOW all the pins of the motors passed
  for (int i = 0; motorIndexes[i] != MOTOR_INDEXES_END_FLAG; i++){
    const MotorData& motor = motorData[motorIndexes[i]];
    digitalWrite(motor.CLOCK_PIN, LOW);
    digitalWrite(motor.COUNTER_PIN, LOW);
  }
}

/*
Checks the state of the power and going variable of the paddleMotorStruct and if both are
true then sets the trasistor's pin to HIGH in order to make the motor move.
*/
void controlPaddleMotorPower(PaddleMotorStruct* paddleMotorController) {
  if(paddleMotorController->power && paddleMotorController->going) {
    digitalWrite(PADDLE_NPN, HIGH);  // Activate the transistor and so make the paddle move
  }else{
    digitalWrite(PADDLE_NPN, LOW);  // Setting the transistor to LOW and so make the paddle stop
  }
}

/*
At each state of the 'paddleMotorStruct.going' corresponds a certain time interval.
Is verified if that interval has been past, if so the state of the 'going' variable is switched
and so the next time that the 'controlPaddleMotorPower' function will be called the motor
will turn off.
*/
void controlPaddleMotorGoing(PaddleMotorStruct* paddleMotorController){
  ul currentMillis = millis();  // Get current time
  // Checking if it's time to switch the going state of the paddle
  if (currentMillis - previousMillis >= (paddleMotorController->going ? paddleGoingInterval : paddleNotGoingInterval)) {
    previousMillis = currentMillis;  // Upadate the last time that the switch has happend
    paddleMotorController->going = !paddleMotorController->going; // Change the going state
  }
}

// Function to set the offset
void resetOffset(uint8_t motorIndex, uint8_t rotationDirection, ul movementDelay){
  int motors[2] = {(int)motorIndex, MOTOR_INDEXES_END_FLAG};  // Creating a list containg the motor and the 0xFF flag
  const MotorData& motor = motorData[motorIndex];
  delay(200);
  digitalWrite(motor.COUNTER_PIN, rotationDirection);
  digitalWrite(motor.CLOCK_PIN, !rotationDirection);
  delay(movementDelay);  // 150
  turnMotorsOff(motors);
}

/*
The first parameter is the index of the motors to rotate (0 -> disk, 1 -> cross);
The second one indicates in which direction the rotation must be done;
The third one is the times that the motor must rotate (1 times -> 1 hall detections, 2 times -> 2 hall detections...);
*/
void rotateMotor(uint8_t motorIndex, uint8_t rotationDirection, uint8_t times) {
  int motors[2] = {(int)motorIndex, MOTOR_INDEXES_END_FLAG};  // Creating a list containg the motor and the 0xFF flag
  for (int i = 0; i < times; i++) {
    const MotorData& motor = motorData[motorIndex];
    // Move the motor away from the magnet or else the hall will detect it and the rotation won't be done
    digitalWrite(motor.COUNTER_PIN, rotationDirection);
    digitalWrite(motor.CLOCK_PIN, !rotationDirection);
    delay(rotationDelay);
    // Now it can start rotating until a magnet is found from the hall
    while (hallCheck(motorData[motorIndex].HALL)) {
      digitalWrite(motor.COUNTER_PIN, rotationDirection);
      digitalWrite(motor.CLOCK_PIN, !rotationDirection);
    }
    turnMotorsOff(motors);  // When a magnet is detected than the motor can be turned off
  }
}

// Function that moves the disk and cross motor simultaneously
void rotateMotorSIM(uint8_t rotationDirectionDisk, uint8_t rotationDirectionCross, uint8_t times) {
  int motors[3] = {0, 1, MOTOR_INDEXES_END_FLAG};  // Creating a list containg the motor and the 0xFF flag
  for (int i = 0; i < times; i++) {
    // Move the motors away from the magnets or else the halls will detect it and the rotation won't be done
    digitalWrite(motorData[0].COUNTER_PIN, rotationDirectionDisk);  // This and next line moves the disk's motor
    digitalWrite(motorData[0].CLOCK_PIN, !rotationDirectionDisk);
    digitalWrite(motorData[1].COUNTER_PIN, rotationDirectionCross);  // This and net line moves the cross's motor
    digitalWrite(motorData[1].CLOCK_PIN, !rotationDirectionCross);
    delay(rotationDelay);
    // Now it can start rotating until a magnet is found from the hall
    while (true){
      digitalWrite(motorData[0].COUNTER_PIN, rotationDirectionDisk);
      digitalWrite(motorData[0].CLOCK_PIN, !rotationDirectionDisk);
      digitalWrite(motorData[1].COUNTER_PIN, rotationDirectionCross);
      digitalWrite(motorData[1].CLOCK_PIN, !rotationDirectionCross);
      if(!(hallCheck(motorData[0].HALL))){
        turnMotorsOff((const int[]){0, MOTOR_INDEXES_END_FLAG});
        while (hallCheck(motorData[1].HALL)) {
          digitalWrite(motorData[1].COUNTER_PIN, rotationDirectionCross);
          digitalWrite(motorData[1].CLOCK_PIN, !rotationDirectionCross);
        }
        break;
      }
      if(!(hallCheck(motorData[1].HALL))){
        turnMotorsOff((const int[]){1, MOTOR_INDEXES_END_FLAG});
        while (hallCheck(motorData[0].HALL)) {
          digitalWrite(motorData[0].COUNTER_PIN, rotationDirectionDisk);
          digitalWrite(motorData[0].CLOCK_PIN, !rotationDirectionDisk);
        }
        break;
      }      
    }
    turnMotorsOff(motors);
  }
}

// Throw function for disposing both plastic trashes or metal trashes
void throwPOM(TrashType trashType){
  uint8_t motorIndex = (trashType == TRASH_METAL) ? 0 : 1;  // 0 is the motor index for the disk, 1 the motor index for the cross
  uint8_t rotationDirection = (trashType == TRASH_METAL) ? COUNTER_CLOCKWISE : CLOCKWISE;
  rotateMotor(motorIndex, rotationDirection, 1);
  delay(serialDelay);
  rotateMotor(motorIndex, !rotationDirection, 1);
  uint8_t offsetDelay = (trashType == TRASH_PLASTIC) ? 150 : 95;
  resetOffset(motorIndex, rotationDirection, offsetDelay);
  trash = TRASH_NONE;
}

/*
If there is no paper trash yet, it simply moves the new one and wait
for a second trash to dispose them in the same movement.
If a paper trash is already present then moves both the old and the new
trash and then dispose both of them.
*/
void throwPaper(){
  if(paperAlreadyPresent){
    rotateMotorSIM(CLOCKWISE, COUNTER_CLOCKWISE, 1);
    rotateMotor(1, COUNTER_CLOCKWISE, 1);
    rotateMotor(1, CLOCKWISE, 1);
    rotateMotorSIM(COUNTER_CLOCKWISE, CLOCKWISE, 1);
    paperAlreadyPresent = false;
  } else {
    rotateMotor(1, COUNTER_CLOCKWISE, 1);
    paperAlreadyPresent = true;
  }
  trash = TRASH_NONE;
}

// RPI4 COMMUNICATION
// Get trash from Rpi4
int getTrashFromPi() {  
  Serial.flush();  // Clear the outgoing buffer
  int trashGet = TRASH_NONE;
  if (Serial.available() > 0) {
    trashGet = Serial.parseInt();  // Get serial input
    if ((trashGet < TRASH_NONE || trashGet > TRASH_PAPER) && trashGet != TRASH_INCOMING) {
      trashGet = TRASH_NONE;  // Reset to default if invalid
    }
  }
  return trashGet;  // Return serial input
}

// Send the feedback to Rpi4
void sendFeedbackToPi(int feedbackNumber){
  Serial.println(feedbackNumber);  // Send the "done" flag to the Rpi4
  Serial.flush();  // Clear the outgoing buffer
  delay(serialDelay);
  while (Serial.available() > 0) { // Clear any remaining data in the input buffer
    Serial.read();
  }
  isThrowing = false;
}
