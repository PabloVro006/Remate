#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// DEFINITION
// Definition for the disk's motor
#define CLOCK_DISK_PIN 2
#define COUNTER_DISK_PIN 4
// Definition for the cross's motor
#define CLOCK_CROSS_PIN 7
#define COUNTER_CROSS_PIN 8
// Definition for the paddle's motor
#define PADDLE_NPN 12
// Definition for the hall's
#define HALL_DISK A0
#define HALL_CROSS A1
// Definitions for the directions of rotation
#define CLOCKWISE 0
#define COUNTER_CLOCKWISE 1
// Definition for the motors indexes
#define DISK 0
#define CROSS 1
// Useful definition
#define MOTOR_INDEXES_END_FLAG 0xFF  // This will be useful for turning the motors off

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

// CONSTANTS
extern ul serialDelay;               // Millis for Serial related actions
extern ul rotationDelay;             // Millis for making sure that the magnet has moved away from the hall
extern ul crossOffsetDelay;          // Millis for adjusting the cross after a rotation
extern ul diskOffsetDelay;           // Millis for adjusting the disk after a rotation
extern const int hallThresholdLow;   // If hall's value < than this, there is a magnet
extern const int hallThresholdHigh;  // If hall's value > than this, there is a magnet
extern const int feedbackOk;         // Number to send at the Rpi4 when throwing is over
// Logic for the paddle motor going delay -> // 200 and 675 with 6v
extern ul paddleGoingInterval;       // Millis indicating the time of paddle's going
extern ul paddleNotGoingInterval;    // Millis indicating the time of paddle's stopping
extern ul trashIncomingTimeout;      // Millis for exiting the 9 condition if nothing is received
extern ul previousMillis;            // Stores the last time that the paddle's going variable has changed

// TRASHING SETUP
extern bool paperAlreadyPresent;     // True when there is already a paper trash type waiting for being disposed
extern bool isThrowing;              // When this is false the arduino read from the Serial, if true then all the data incoming from the Serial won't be considered
extern TrashType trash;              // Trash initialized as null

// MOTOR STRUCTS
// Struct for the disk's and the cross's motor
typedef struct {
  int COUNTER_PIN;
  int CLOCK_PIN;
  int HALL;
} MotorData;
extern const MotorData motorData[];  // List that assigns to each element of the struct a pin

// Struct for the paddle's motor
typedef struct {
  bool power;  // Indicates if the paddle should be moving (true) or not (false)
  bool going;  // Indicates if the paddle is currently in the "going" or "not going" position when power is true
} PaddleMotorStruct;
extern PaddleMotorStruct paddleMotorStruct; // Instance of PaddleMotorStruct to control the paddle motor

// DECLEARING FUNCTIONS
bool hallCheck(int hall);                                                // Read values from disk's or cross's hall
void turnMotorsOff(const int motorIndexes[]);                            // Turn off motors passed in the array
void controlPaddleMotorPower(PaddleMotorStruct* paddleMotorController);  // Main function for the paddle motor's transistor handling
void controlPaddleMotorGoing(PaddleMotorStruct* paddleMotorController);  // Control paddle motor's going state depending on time
void resetMotorOffset(uint8_t motorIndex, uint8_t rotationDirection, ul movementDelay);  // Adjusts the motor's offset after a rotation has occurred
void rotateMotor(uint8_t motorIndex, uint8_t rotationDirection, uint8_t times);  // Rotate cross's or disk's motor
void rotateMotorSIM(uint8_t rotationDirectionDisk, uint8_t rotationDirectionCross);  // Rotate simultaneously the cross's and the disk's motor
void throwPOM(TrashType trashType);                                      // Throw POM (plastic or metal)
void throwPaper();                                                       // Function to handle paper trashes 
int getTrashFromPi();                                                    // Get the Serial input from Rpi4
void sendFeedbackToPi(int feedbackNumber);                               // Send the Serial feedback to Rpi4

#endif /*CONFIG_H*/
