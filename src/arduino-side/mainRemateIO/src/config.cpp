// Include config header file
#include "config.h"

// DEFINE VARIABLES
ul serialDelay = 20;
ul rotationDelay = 900;
ul crossOffsetDelay = 170;
ul diskOffsetDelay = 95;
const int hallThresholdLow = 400;
const int hallThresholdHigh = 550;
const int feedbackOk = 42;
ul paddleGoingInterval = 70;  // When there is no friction, this is too much
ul paddleNotGoingInterval = 1000;
ul trashIncomingTimeout = 5000;
ul previousMillis = 0;
bool paperAlreadyPresent = false;
bool isThrowing = false;
TrashType trash = TRASH_NONE;
const MotorData motorData[] = {
  {COUNTER_DISK_PIN, CLOCK_DISK_PIN, HALL_DISK},
  {COUNTER_CROSS_PIN, CLOCK_CROSS_PIN, HALL_CROSS}
};
PaddleMotorStruct paddleMotorStruct = {1, 1};  // Initialize this at {1, 1} to make the paddle move

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
    const MotorData& motor = motorData[motorIndexes[i]];  // Creating the 'motor' variable
    digitalWrite(motor.CLOCK_PIN, LOW);
    digitalWrite(motor.COUNTER_PIN, LOW);
  }
}

/*
 This is the main function for the paddle's motor handling. Depending on the 'power' and 'going'
 variables, turns on or off the motor. If both are true than the transistor's pin is set to HIGH
 in order to make the paddle's motor move.
*/
void controlPaddleMotorPower(PaddleMotorStruct* paddleMotorController) {
  if(paddleMotorController->power && paddleMotorController->going) {  // Checks if both of the variables are true
    digitalWrite(PADDLE_NPN, HIGH);  // Setting the transistor to HIGH and so make the paddle's motor move
  } else {
    digitalWrite(PADDLE_NPN, LOW);  // Setting the transistor to LOW and so make the paddle'motor stop
  }
}

/*
 At each state of the 'paddleMotorStruct.going' corresponds a certain time interval.
     going = 1 -> interval = 90, going = 0 -> interval = 1000
 Is verified if that interval has been past, if so the state of the 'going' variable is switched.
 The next time that the 'controlPaddleMotorPower' function will be called the motor will act
 accordingly.
 (if going is switched to false, then when the 'controlPaddleMotorPower' will be called, the motor will turn off).
*/
void controlPaddleMotorGoing(PaddleMotorStruct* paddleMotorController){
  ul currentMillis = millis();  // Get current time
  // Checking if it's time to switch the going state of the paddle
  if (currentMillis - previousMillis >= (paddleMotorController->going ? paddleGoingInterval : paddleNotGoingInterval)) {  // Decides which interval to consider
    previousMillis = currentMillis;  // Upadate the last time that the switch has happend
    paddleMotorController->going = !paddleMotorController->going;  // Change the going state
  }
}

/*
 Due to the high speed and acceleration of the motors and the lack of precise mounting, when the motor turns off
 the disk or the cross keeps moving for a bit. This function is used to bring back the cross or disk and to adjust
 the offset.
*/
void resetMotorOffset(uint8_t motorIndex, uint8_t rotationDirection, ul movementDelay){
  int motors[2] = {(int)motorIndex, MOTOR_INDEXES_END_FLAG};  // Creating a list containg the motor and the 0xFF flag
  const MotorData& motor = motorData[motorIndex];  // Creating the 'motor' variable
  delay(200);  // Wait for the cross or disk to stop moving
  digitalWrite(motor.COUNTER_PIN, rotationDirection);
  digitalWrite(motor.CLOCK_PIN, !rotationDirection);
  delay(movementDelay);  // Short time for adjusting the offset
  turnMotorsOff(motors);
}

/*
 The first parameter is the index of the motors to rotate;
 The second one indicates in which direction the rotation must be done;
 The third one is the times that the motor must rotate (1 times -> 1 hall detections, 2 times -> 2 hall detections...);
*/
void rotateMotor(uint8_t motorIndex, uint8_t rotationDirection, uint8_t times) {
  int motors[2] = {(int)motorIndex, MOTOR_INDEXES_END_FLAG};  // Creating a list containg the motor and the 0xFF flag
  for (int i = 0; i < times; i++) {
    const MotorData& motor = motorData[motorIndex];  // Creating the 'motor' variable
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

/*
 This function moves both the motor simultaneously regardless of the halls.
 It's used for the first part of the simultaneous rotation where i just need
 to move the cross and the disk to make the magnet moves from the hall.
*/
void enableMotorSIM(const uint8_t directions[]){
  for (int i = 0; i < 2; i++){
    digitalWrite(motorData[1 - i].COUNTER_PIN, directions[1 - i]);
    digitalWrite(motorData[1 - i].CLOCK_PIN, !directions[1 - i]);
  }
}

// Function that moves the disk and cross motor simultaneously
void rotateMotorSIM(uint8_t rotationDirectionDisk, uint8_t rotationDirectionCross) {
  int motors[3] = {DISK, CROSS, MOTOR_INDEXES_END_FLAG};  // Creating a list containg the motor and the 0xFF flag
  uint8_t directions[2] = {rotationDirectionDisk, rotationDirectionCross};  // Creating a list containing the directions in order
  // Move the motors away from the magnets or else the halls will detect it and the rotation won't be done
  enableMotorSIM(directions);
  delay(rotationDelay);
  // Now it can start rotating until a magnet is found from the hall
  bool exitLoop = false;  // Boolean for exiting the loop when both the halls detects a magnet
  while (!exitLoop) {
    enableMotorSIM(directions);  // Move both of them until on the halls detect a magnet
    // Check for magnets and handle motor stops
    for (int i = 0; i < 2; i++) {
      if (!hallCheck(motorData[i].HALL)) {  // Checks for both the cross and the disk, based on the i value
        const int motorToTurnOff[] = {i, MOTOR_INDEXES_END_FLAG};  // Turns off the motor, which hall found a magnet
        turnMotorsOff(motorToTurnOff);
        // Rotate the other motor until its magnet is detected
        while (hallCheck(motorData[1 - i].HALL)) {
          digitalWrite(motorData[1 - i].COUNTER_PIN, directions[1 - i]);
          digitalWrite(motorData[1 - i].CLOCK_PIN, !directions[1 - i]);
        }
        exitLoop = true;  // Exit the while loop after both motors are aligned with magnets
        break;  // Exit the for loop
      }
    }
  }
  turnMotorsOff(motors);  // Turn off all the motors
}

// This function is used for dispose both the TRASH_PLASTIC and TRASH_METAL.
void throwPOM(TrashType trashType){
  uint8_t motorIndex = (trashType == TRASH_METAL) ? DISK : CROSS;  // Choosing the motorIndex based on the trashType
  uint8_t rotationDirection = (trashType == TRASH_METAL) ? COUNTER_CLOCKWISE : CLOCKWISE;  // Choosing the rotationDirection based on the trashType
  rotateMotor(motorIndex, rotationDirection, 1);  // Moves the motor to put the trash in the corresponding bin
  delay(serialDelay);
  rotateMotor(motorIndex, !rotationDirection, 1); // Makes the motor go back in place
  resetMotorOffset(motorIndex, rotationDirection, (trashType == TRASH_PLASTIC ? crossOffsetDelay : diskOffsetDelay));  // Adjusting the offset post-rotation
  trash = TRASH_NONE;  // Re-setting the 'trash' variable
}

/*
 If there is no paper waste yet, it simply moves the new one and wait
 for a second waste to dispose them in the same movement.
 If a paper waste is already present then moves both the old and the new
 waste and then dispose both of them.
*/
void throwPaper(){
  if(paperAlreadyPresent){
    rotateMotorSIM(CLOCKWISE, COUNTER_CLOCKWISE);  // Simultaneously rotates both the disk's motor and the cross's motor
    resetMotorOffset(DISK, COUNTER_CLOCKWISE, diskOffsetDelay);  // Adjusting the disk's offset
    rotateMotor(CROSS, COUNTER_CLOCKWISE, 1);  // Now the cross rotates again to dispose also the second-arrived waste
    delay(serialDelay);
    rotateMotor(CROSS, CLOCKWISE, 1);  // Once both the new waste and the previous waste have been thrown away, the cross starts to go back in place
    rotateMotorSIM(COUNTER_CLOCKWISE, CLOCKWISE); // Simultaneously rotates both the disk's motor and the cross's motor
    resetMotorOffset(CROSS, COUNTER_CLOCKWISE, crossOffsetDelay);  // Adjusting the cross's offset
    resetMotorOffset(DISK, CLOCKWISE, diskOffsetDelay);  // Adjusting the disk's offset
    paperAlreadyPresent = false;  // Now this becomes true because both wastes have been disposed
  } else {  // If there is no paper waste yet
    rotateMotor(CROSS, COUNTER_CLOCKWISE, 1);  // Rotate the cross to moves the waste
    resetMotorOffset(CROSS, CLOCKWISE, crossOffsetDelay);  // Adjusting the cross's offset
    paperAlreadyPresent = true;  // Now the 'paperAlreadyPresent' is true
  }
  trash = TRASH_NONE;
}

/*
 When more than one type of trash falls then they all get disposed in a particular bi
*/
void throwUnsorted(){
	rotateMotorSIM(CLOCKWISE, COUNTER_CLOCKWISE);  // Simultaneously rotates both the disk's motor and the cross's motor
	resetMotorOffset(CROSS, CLOCKWISE, crossOffsetDelay);  // Adjusting the cross's offset
	rotateMotor(DISK, CLOCKWISE, 1);  //
	delay(serialDelay);
	rotateMotor(DISK, COUNTER_CLOCKWISE, 1);  //
	rotateMotorSIM(COUNTER_CLOCKWISE, CLOCKWISE);  // Simultaneously rotates both the disk's motor and the cross's motor
	resetMotorOffset(CROSS, COUNTER_CLOCKWISE, crossOffsetDelay);  // Adjusting the cross's offset
	resetMotorOffset(DISK, CLOCKWISE, diskOffsetDelay);  // Adjusting the disk's offset
}

// RPI4 COMMUNICATION
// Function to verify that the incoming data from the Rpi4 is a valid number
bool isValidTrashType(TrashType trashToVerify) {
  return (trashToVerify > TRASH_NONE && trashToVerify <= TRASH_UNSORTED) || trashToVerify == TRASH_INCOMING;
}

/*
 If the Rpi4 is sending something through the Serial port, read it, convert it in int
 type and returns it (only if it is a value between 0 and 3 or equal to 9).
*/
int getTrashFromPi() {
  Serial.flush();  // Clear the outgoing buffer
  TrashType trashGet = TRASH_NONE;
  if (Serial.available() > 0) {  // Verify that something is coming through the Serial port
    trashGet = TrashType(Serial.parseInt());  // Get serial input
    if (!isValidTrashType(trashGet)) {  // Checks if it is a valid value
      trashGet = TRASH_NONE;  // Reset to default if invalid
    }
  }
  return trashGet;  // Return the verified value
}

// Send the feedback to Rpi4 after the throwing is over
void sendFeedbackToPi(int feedbackNumber){
  Serial.println(feedbackNumber);  // Send the "done" flag (42) to the Rpi4
  Serial.flush();  // Clear the outgoing buffer
  delay(serialDelay);
  while (Serial.available() > 0) { // Clear any remaining data in the input buffer
    Serial.read();
  }
  isThrowing = false;
}
