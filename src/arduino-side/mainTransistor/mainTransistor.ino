// TRANSISTOR DEFINITION
// Definition for the disk's motor
#define CLOCK_DISK_PNP 2
#define COUNTER_DISK_PNP 3
#define CLOCK_DISK_NPN 4
#define COUNTER_DISK_NPN 5
// Definition for the cross's motor
#define CLOCK_CROSS_PNP 6
#define COUNTER_CROSS_PNP 7
#define CLOCK_CROSS_NPN 8
#define COUNTER_CROSS_NPN 9
// Definition for the paddle motor
#define PADDLE_NPN 12

// Useful definition
#define MOTOR_INDEX_END_FLAG 0xFF
#define CLOCKWISE 0
#define COUNTER_CLOCKWISE 1

// Typedefinition for unsigned long type
typedef unsigned long ul;

// ENUM FOR TRASH TYPES
enum TrashType {
  TRASH_NONE = 0,
  TRASH_METAL = 1,
  TRASH_PLASTIC = 2,
  TRASH_PAPER = 3,
  TRASH_INCOMING = 9,  // 9 is a flag indicating that something has been detected from the Rpi4, but it's not defined yet
};

// HALL DEFINITION
const int HALL_DISK = A0;
const int HALL_CROSS = A1;

// CONSTANTS
const int serialDelay = 20;
const int hallThresholdLow = 400;  // If hall's value < than this, there is a magnet 
const int hallThresholdHigh = 550; // If hall's value > thas this, there is amagnet
const int rotationDelay = 1000;  // Millis for making sure that the magnet has moved away from the hall
const int feedbackOk = 42;         // Number to send at the Rpi4 when throwing is over
// Assign variables to represent specific trash types
const TrashType trashTypeMetal = TRASH_METAL;
const TrashType trashTypePlastic = TRASH_PLASTIC;
// Logic for the paddle motor going delay
const long paddleGoingInterval = 200;    // Millis indicating the time of paddle's going
const long paddleNotGoingInterval = 675; // Millis indicating the time of paddle's stopping
const long trashIncomingTimeout = 5000;  // Millis for exiting the 9 condition if nothing is received
ul previousMillis = 0;        // Stores the last time the switch of the paddle's going was changed

// TRASHING SETUP
bool paperAlreadyPresent = false; // True when there is a trash paper type waiting for being disposed
bool isThrowing = false;  // When this is false the arduino read from Serial
int trash = TRASH_NONE;   // Trash initialized ad null

// MOTOR STRUCT
// Struct for the disk's and the cross's motor
typedef struct {
  int CLOCK_NPN;
  int CLOCK_PNP;
  int COUNTER_NPN;
  int COUNTER_PNP;
  int HALL;
} MotorData;
// Array that assigns transistor pins and Hall sensor pins to each motor (disk and cross)
static const MotorData motorData[2] = {
  {CLOCK_DISK_NPN, CLOCK_DISK_PNP, COUNTER_DISK_NPN, COUNTER_DISK_PNP, HALL_DISK},
  {CLOCK_CROSS_NPN, CLOCK_CROSS_PNP, COUNTER_CROSS_NPN, COUNTER_CROSS_PNP, HALL_CROSS}
};
// Struct for the paddle's motor
typedef struct {
  bool power;  // Indicates if the paddle should be moving (true) or not (false)
  bool going;  // Indicates if the paddle is currently in the "going" or "not going" position when power is true
} PaddleMotorStruct;
PaddleMotorStruct paddleMotorStruct; // Instance of PaddleMotorStruct to control the paddle motor

// DECLEARING FUNCTIONS
bool hallCheck(int hall);                                                // Check for disk's or cross's hall state
void turnMotorsOff(const int motorIndexes[]);                            // Turn off motor passed in the array
void controlPaddleMotorPower(PaddleMotorStruct* paddleMotorController);  // Main function for the paddle motor's transistor handling
void controlPaddleMotorGoing(PaddleMotorStruct* paddleMotorController);  // Control paddle motor going state depending on time
void rotateMotor(uint8_t motorIndex, uint8_t direction, uint8_t times);  // Rotate cross's or disk's motor
void throwTrash(TrashType trashType);                                    // Throw trash
int getTrashFromPi();                                                    // Get the Serial input from Rpi4
void sendFeedbackToPi(int feedbackNumber);                               // Send the Serial feedback to Rpi4

// SETUP
void setup() {
  // SERIAL INITIALIZATION
  delay(1000);
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
    // Set the transistors pin to output
    pinMode(motorData[i].CLOCK_NPN, OUTPUT);
    pinMode(motorData[i].CLOCK_PNP, OUTPUT);
    pinMode(motorData[i].COUNTER_NPN, OUTPUT);
    pinMode(motorData[i].COUNTER_PNP, OUTPUT);
    // Turning off the transistors
    digitalWrite(motorData[i].CLOCK_NPN, LOW);
    digitalWrite(motorData[i].CLOCK_PNP, LOW);
    digitalWrite(motorData[i].COUNTER_NPN, LOW);
    digitalWrite(motorData[i].COUNTER_PNP, LOW);
  }
  // Make the paddle move
  paddleMotorStruct.power = 1;
  paddleMotorStruct.going = 1;

  // CALIBRATION
  //rotateMotor(0, COUNTER_CLOCKWISE, 4); // Disk calibration
  rotateMotor(1, CLOCKWISE, 1); // Cross calibration
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
      isThrowing = true;  // This is useful for stopping the data reception form the Rpi4
      trash == TRASH_METAL ? throwTrash(trashTypeMetal) : throwTrash(trashTypePlastic);
      // Send feedbakc to Rpi4 and start moving the paddle again
      sendFeedbackToPi(feedbackOk);
      paddleMotorStruct.power = 1;
      paddleMotorStruct.going = 0;
    }
  }
  // Call the function for moving the paddle based on the parameters that as been changed
  controlPaddleMotorPower(&paddleMotorStruct);
}

// DEFINE FUNCTIONS
// Hall reading function
bool hallCheck(int hall) {
  delay(serialDelay);
  int reading = analogRead(hall);  // Read the output of the hall passed
  return (reading > hallThresholdLow && reading < hallThresholdHigh);  // Check if a magnet is detected
}

// Turn off motors
/*
The parameter is a list containg all the motor's indexes that has to be turned off.
The last value of the list is a 0xFF flag, indicating the end for the loop.
*/
void turnMotorsOff(const int motorIndexes[]){
  // Setting to LOW all the transistor of the motors passed
	for (int i=0; motorIndexes[i] != MOTOR_INDEX_END_FLAG; i++){
    const MotorData& motor = motorData[motorIndexes[i]];
    digitalWrite(motor.CLOCK_NPN, LOW);
    digitalWrite(motor.CLOCK_PNP, LOW);
    digitalWrite(motor.COUNTER_NPN, LOW);
    digitalWrite(motor.COUNTER_PNP, LOW);
	}
}

// Main function for the paddle motor's transistor handling
void controlPaddleMotorPower(PaddleMotorStruct* paddleMotorController) {
  if(motorController->power && motorController->going) {
    digitalWrite(PADDLE_NPN, HIGH);  // Activate the transistor and so make the paddle move
  }else{
    digitalWrite(PADDLE_NPN, LOW);  // Setting the transistor to LOW and so make the paddle stop
  }
}

// Check if the interval corrisponding to the going state is over or not, and if so, switches the state
void controlPaddleMotorGoing(PaddleMotorStruct* paddleMotorController){
  ul currentMillis = millis();  // Get current time
  // Checking if it's time to switch the going state of the paddle
  if (currentMillis - previousMillis >= (paddleMotorStruct->going ? paddleGoingInterval : paddleNotGoingInterval)) {
    previousMillis = currentMillis;
    paddleMotorController->going = !paddleMotorController->going; // Change the going state
  }
}

// Generic function to move motors
/*
The first parameter is the index of the motors to rotate (0 -> disk, 1 -> cross)
The third one is the times that the motor must rotate (1 times -> 1 hall detections, 2 times -> 2 hall detections...)
*/
void rotateMotor(uint8_t motorIndex, uint8_t rotationDirection, uint8_t times) {
  int motors[2] = {(int)motorIndex, MOTOR_INDEX_END_FLAG};  // Creating a list containg the motor and the 0xFF flag
  // Move the motor away from the magnet or else the hall will detect it and the rotation won't be done
  for (int i = 0; i < times; i++) {
    digitalWrite(motorData[motorIndex].COUNTER_RELAY, rotationDirection);
    digitalWrite(motorData[motorIndex].CLOCK_RELAY, !rotationDirection);
    delay(rotationDelay);
    // Now it can start rotating until a magnet is found from the hall
    while (hallCheck(motorData[motorIndex].HALL)) {
      digitalWrite(motorData[motorIndex].COUNTER_RELAY, rotationDirection);
      digitalWrite(motorData[motorIndex].CLOCK_RELAY, !rotationDirection);
    }
    turnMotorsOff(motors);
  }
}

// Throw function
void throwTrash(TrashType trashType){
  uint8_t motorIndex = (trashType == TRASH_METAL) ? CLOCKWISE : COUNTER_CLOCKWISE;
  uint8_t order = (trashType == TRASH_METAL) ? COUNTER_CLOCKWISE : CLOCKWISE;
  rotateMotor(motorIndex,order,1);
  // If the motor moved is the cross there is no need to move it back again
  if(trashType == TRASH_METAL){
    rotateMotor(motorIndex,!order,1);
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
    if ((trashGet < TRASH_NONE || trashGet > TRASH_PLASTIC) && trashGet != TRASH_INCOMING) {
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