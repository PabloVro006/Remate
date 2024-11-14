#ifndef INC_CONFIG_H
#define INC_CONFIG_H

// DEFINITION
// Definition for the disk's motor
#define CLOCK_DISK_PIN 2
#define COUNTER_DISK_PIN 4
// Definition for the cross's motor
#define CLOCK_CROSS_PIN 7
#define COUNTER_CROSS_PIN 8
// Definition for the paddle's motor
#define PADDLE_NPN 12
// Useful definition
#define MOTOR_INDEXES_END_FLAG 0xFF
#define CLOCKWISE 0
#define COUNTER_CLOCKWISE 1

// Typedefinition for unsigned long type
typedef unsigned long ul;

// Enum for trash types
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
const int hallThresholdHigh = 550; // If hall's value > than this, there is a magnet
const int rotationDelay = 1000;    // Millis for making sure that the magnet has moved away from the hall
const int feedbackOk = 42;         // Number to send at the Rpi4 when throwing is over
// Logic for the paddle motor going delay
const long paddleGoingInterval = 90;      // Millis indicating the time of paddle's going
// 200 and 675 with 6v
const long paddleNotGoingInterval = 1000; // Millis indicating the time of paddle's stopping
const long trashIncomingTimeout = 5000;   // Millis for exiting the 9 condition if nothing is received
ul previousMillis = 0;                    // Stores the last time the switch of the paddle's going was changed

// TRASHING SETUP
bool paperAlreadyPresent = false; // True when there is a paper trash type waiting for being disposed
bool isThrowing = false;          // When this is false the arduino read from Serial
int trash = TRASH_NONE;           // Trash initialized as null

// MOTOR STRUCTS
// Struct for the disk's and the cross's motor
typedef struct {
  int COUNTER_PIN;
  int CLOCK_PIN;
  int HALL;
} MotorData;
static const MotorData motorData[] = {
  {COUNTER_DISK_PIN, CLOCK_DISK_PIN, HALL_DISK},
  {COUNTER_CROSS_PIN, CLOCK_CROSS_PIN, HALL_CROSS}
};

// Struct for the paddle's motor
typedef struct {
  bool power;  // Indicates if the paddle should be moving (true) or not (false)
  bool going;  // Indicates if the paddle is currently in the "going" or "not going" position when power is true
} PaddleMotorStruct;
PaddleMotorStruct paddleMotorStruct; // Instance of PaddleMotorStruct to control the paddle motor

// DECLEARING FUNCTIONS
bool hallCheck(int hall);                                                // Read values from disk's or cross's hall
void turnMotorsOff(const int motorIndexes[]);                            // Turn off motors passed in the array
void controlPaddleMotorPower(PaddleMotorStruct* paddleMotorController);  // Main function for the paddle motor's transistor handling
void controlPaddleMotorGoing(PaddleMotorStruct* paddleMotorController);  // Control paddle motor going state depending on time
void resetOffset(uint8_t motorIndex, uint8_t rotationDirection);         // CHOOSE DESCRITION AND NAME(?)
void rotateMotor(uint8_t motorIndex, uint8_t direction, uint8_t times);  // Rotate cross's or disk's motor
void rotateMotorSIM(uint8_t rotationDirectionDisk, uint8_t rotationDirectionCross, uint8_t times);  // Rotate simultaneously the cross's and the disk's motor
void throwPOM(TrashType trashType);                                      // Throw POM (plastic or metal)
void throwPaper();                                                       // Function to handle paper trashes 
int getTrashFromPi();                                                    // Get the Serial input from Rpi4
void sendFeedbackToPi(int feedbackNumber);                               // Send the Serial feedback to Rpi4

#endif
