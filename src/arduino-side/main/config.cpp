// Include config header file
#include "config.h"

// DEFINE VARIABLES
const int HALL_DISK = A0;
const int HALL_CROSS = A1;
ul serialDelay = 20;
ul rotationDelay = 1000;
ul crossOffsetDelay = 150;
ul diskOffsetDelay = 95;
const int hallThresholdLow = 400;
const int hallThresholdHigh = 550;
const int feedbackOk = 42;
ul paddleGoingInterval = 90;
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
PaddleMotorStruct paddleMotorStruct = {1, 1};

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
  } else {
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
  delay(movementDelay);
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
  int motors[3] = {DISK, CROSS, MOTOR_INDEXES_END_FLAG};  // Creating a list containg the motor and the 0xFF flag
  for (int i = 0; i < times; i++) {
    // Move the motors away from the magnets or else the halls will detect it and the rotation won't be done
    digitalWrite(motorData[DISK].COUNTER_PIN, rotationDirectionDisk);  // This and next line moves the disk's motor
    digitalWrite(motorData[DISK].CLOCK_PIN, !rotationDirectionDisk);
    digitalWrite(motorData[CROSS].COUNTER_PIN, rotationDirectionCross);  // This and net line moves the cross's motor
    digitalWrite(motorData[CROSS].CLOCK_PIN, !rotationDirectionCross);
    delay(rotationDelay);
    // Now it can start rotating until a magnet is found from the hall
    while (true){
      digitalWrite(motorData[DISK].COUNTER_PIN, rotationDirectionDisk);
      digitalWrite(motorData[DISK].CLOCK_PIN, !rotationDirectionDisk);
      digitalWrite(motorData[CROSS].COUNTER_PIN, rotationDirectionCross);
      digitalWrite(motorData[CROSS].CLOCK_PIN, !rotationDirectionCross);
      if(!(hallCheck(motorData[DISK].HALL))){
        turnMotorsOff((const int[]){DISK, MOTOR_INDEXES_END_FLAG});
        while (hallCheck(motorData[CROSS].HALL)) {
          digitalWrite(motorData[CROSS].COUNTER_PIN, rotationDirectionCross);
          digitalWrite(motorData[CROSS].CLOCK_PIN, !rotationDirectionCross);
        }
        break;
      }
      if(!(hallCheck(motorData[CROSS].HALL))){
        turnMotorsOff((const int[]){CROSS, MOTOR_INDEXES_END_FLAG});
        while (hallCheck(motorData[DISK].HALL)) {
          digitalWrite(motorData[DISK].COUNTER_PIN, rotationDirectionDisk);
          digitalWrite(motorData[DISK].CLOCK_PIN, !rotationDirectionDisk);
        }
        break;
      }      
    }
    turnMotorsOff(motors);
  }
}

// Throw function for disposing both plastic trashes and metal trashes
void throwPOM(TrashType trashType){
  uint8_t motorIndex = (trashType == TRASH_METAL) ? DISK : CROSS;  // 0 is the motor index for the disk, 1 the motor index for the cross
  uint8_t rotationDirection = (trashType == TRASH_METAL) ? COUNTER_CLOCKWISE : CLOCKWISE;
  rotateMotor(motorIndex, rotationDirection, 1);
  delay(serialDelay);
  rotateMotor(motorIndex, !rotationDirection, 1);
  resetOffset(motorIndex, rotationDirection, (trashType == TRASH_PLASTIC ? crossOffsetDelay : diskOffsetDelay));
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
    rotateMotor(CROSS, COUNTER_CLOCKWISE, 1);
    rotateMotor(CROSS, CLOCKWISE, 1);
    rotateMotorSIM(COUNTER_CLOCKWISE, CLOCKWISE, 1);
    paperAlreadyPresent = false;
  } else {
    rotateMotor(CROSS, COUNTER_CLOCKWISE, 1);
    resetOffset(CROSS, CLOCKWISE, crossOffsetDelay);
    paperAlreadyPresent = true;
  }
  trash = TRASH_NONE;
}

// RPI4 COMMUNICATION
// Read from serial if something is coming and only accepts values between 0 and 3 or equal to 9
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
