/*
// LIBRARIES
#include <LiquidCrystal_I2C.h>
// LCD SETUP
LiquidCrystal_I2C lcd(0x27,  16, 2);
int counterDiskRelay = 2;
int clockDiskRelay = 4;
int counterCrossRelay = 8;
int clockCrossRelay = 7;
int paddleRelay = 12;
// LCD DISPLAY INITIALIZATION
lcd.init();
lcd.backlight();
*/

// RELAY SETUP
// Disk relay
#define COUNTER_DISK_RELAY 2
#define CLOCK_DISK_RELAY 4
// Cross relay
#define COUNTER_CROSS_RELAY 8
#define CLOCK_CROSS_RELAY 7
// Paddle relay
#define PADDLE_RELAY 12

// HALL SETUP
// Setting pin
int hallDisk = A0;
int hallCross = A1;

// PADDLE SETUP
int paddleMotorState = 1;

// TRASHING SETUP
// Disk initialization
int diskState[4] = { 0, 0, 0, 5 };  // empty, empty, empty, hole

// DECLEARING FUNCTIONS
bool hallCheck(int hall);                                                      // Check for disk or cross hall
void turnMotorsOff(int relays[]);                                              // Turn off motor passed in the array
void paddleMotor(int motorController);                                         // Control paddle motor
void rotation(int counterRelay, int clockRelay, int rotationDirection);        // Rotate cross or disk
void throwPaper();                                                             // Disk rotate 2 times (counter clock) and goes back
void throwPaperPlasticCombo();                                                 // Disk do a full rotation (clock wise)
void normalThrow(int counterRelay, int clockRelay, int diskSpace, int order);  // Throw plastic or nr
void rotateList();                                                             // Make element in diskState rotate
int getTrashFromPi();                                                          // Get serial input from Rpi4
void sendFeedbackToPi(int feedbackNumber);                                     // Send the feedback to Rpi4

// DEFINING MOTOR STRUCT
// Struct itself
typedef struct {
  int COUNTER_RELAY;
  int CLOCK_RELAY;
  int HALL;
} mstruct;
// Creation of motors using the struct
static const mstruct motorData[] = {
	{
		COUNTER_DISK_RELAY,
		CLOCK_DISK_RELAY,
		hallDisk,
	},
	{
		COUNTER_CROSS_RELAY,
		CLOCK_CROSS_RELAY,
		hallCross,
	}
};

// SETUP
void setup() {
  // Wait a bit
  delay(1000);

  // SERIAL INITIALIZATION
  Serial.begin(115200);

  // HALL INITIALIZATION
  pinMode(hallDisk, INPUT);
  pinMode(hallCross, INPUT);

  // RELAY INITIALIZATION
  // Setting pin to output
  pinMode(counterDiskRelay, OUTPUT);
  pinMode(clockDiskRelay, OUTPUT);
  pinMode(counterCrossRelay, OUTPUT);
  pinMode(clockCrossRelay, OUTPUT);
  pinMode(paddleRelay, OUTPUT);
  // Setting motor to off
  int relays[] = {
    counterDiskRelay,
    clockDiskRelay,
    counterCrossRelay,
    clockCrossRelay,
    paddleRelay,
    -1
  };
  turnMotorsOff(relays);

  // CALIBRATION
  // Disk calibration
  //throwPaperPlasticCombo();
  // Cross calibration
  rotation(counterCrossRelay, clockCrossRelay, 0);
}

// LOOP
void loop() {
  // CONTROL PADDLE
  // Set motor state (off/on)
  paddleMotorState = diskState[0] == 0 ? 1 : 0;

  // Get trash
  int trash = getTrashFromPi();
  // Now control the paddle movement
  if (trash != 0) {
    paddleMotor(0);
    diskState[0] = trash;
  } else {
    paddleMotor(paddleMotorState);
  }

  // INITIAL CHECK
  // Check for paper + plastic combo
  if (diskState[1] == 2 && diskState[0] == 1) {
    paddleMotor(0);
    throwPaperPlasticCombo();
    paddleMotor(paddleMotorState);
    sendFeedbackToPi(42);
  }
  // Check for metal + paper combo
  else if (diskState[2] == 3 && diskState[1] == 2) {
    paddleMotor(0);
    throwPaper();
    paddleMotor(paddleMotorState);
    sendFeedbackToPi(42);
  }
  // Check for metal
  else if (diskState[2] == 3) {
    paddleMotor(0);
    normalThrow(counterDiskRelay, clockDiskRelay, 2, 0);
    paddleMotor(paddleMotorState);
    sendFeedbackToPi(42);
  }
  // Check for paper
  else if (diskState[1] == 2) {
    paddleMotor(0);
    throwPaper();
    paddleMotor(paddleMotorState);
    sendFeedbackToPi(42);
  }

  // NORMAL CASE
  switch (trash) {
    case 1:
      normalThrow(counterDiskRelay, clockDiskRelay, 0, 1);
      sendFeedbackToPi(42);
      break;
    case 2:
      rotation(counterCrossRelay, clockCrossRelay, 1);
      sendFeedbackToPi(42);
      rotateList();
      break;
    case 3:
      rotation(counterCrossRelay, clockCrossRelay, 1);
      sendFeedbackToPi(42);
      rotateList();
      break;
    case 4:
      normalThrow(counterCrossRelay, clockCrossRelay, 0, 0);
      sendFeedbackToPi(42);
      break;
    default:
      delay(10);
  }
}

// DEFINE FUNCTIONS
// Hall reading function
bool hallCheck(int hall) {  // Disk & cross reading
  if (analogRead(hall) > 490 && analogRead(hall) < 550) {
    return true;
  } else {
    return false;
  }
}

// Turn off motor
void turnMotorsOff(int relays[]) {
  for (int i = 0; relays[i] != -1; i++) {
    digitalWrite(relays[i], HIGH);
  }
}

// Paddle movement
void paddleMotor(int motorController) {
  digitalWrite(paddleRelay, motorController);
}

// Disk & cross rotation
void rotation(int counterRelay, int clockRelay, int rotationDirection) {
  // Move the magnet away from the hall sensor
  digitalWrite(counterRelay, !rotationDirection);
  digitalWrite(clockRelay, rotationDirection);
  delay(900);
  // Now that the old magnet is far from sensor the funciton can start search from the next magnet
  int hallToPass = (counterRelay == 2) ? hallDisk : hallCross;  // See whether the sensor to be checked is the disc or the cross one
  while (hallCheck(hallToPass)) {                               // Now the motor can move until the hall found the next magnet
    digitalWrite(counterRelay, !rotationDirection);
    digitalWrite(clockRelay, rotationDirection);
  }
  int relays[] = { counterRelay, clockRelay, -1 };  // Turn off motor
  turnMotorsOff(relays);
}

// THROWING FUNCTION
// Disk rotate 2 times counter wise then goes back
void throwPaper() {
  for (int i = 0; i < 2; i++) {
    rotation(counterDiskRelay, clockDiskRelay, 0);
  }
  for (int i = 0; i < 2; i++) {
    rotation(counterDiskRelay, clockDiskRelay, 1);
  }
  diskState[2] = diskState[1] = 0;  // Update diskState
}
// Disk rotate 4 times clock wise
void throwPaperPlasticCombo() {
  for (int i = 0; i < 4; i++) {
    rotation(counterDiskRelay, clockDiskRelay, 1);
  }
  for (int i = 0; i < 3; i++) {
    diskState[i] = 0;
  }
  diskState[3] = 5;  // Update diskState
}
// Generic function for throw both plastic & nr
void normalThrow(int counterRelay, int clockRelay, int diskSpace, int order) {
  rotation(counterRelay, clockRelay, order);
  rotation(counterRelay, clockRelay, !order);
  diskState[diskSpace] = 0;  // Update diskState
}

// Rotate element in diskState[]
void rotateList() {
  diskState[2] = diskState[1];
  diskState[1] = diskState[0];
  diskState[0] = 0;
}

// RPI4 COMMUNICATION
// Get trash from Rpi4
int getTrashFromPi() {
  int trashGet = 0;
  if (Serial.available() > 0) {
    // Turn off paddle motor until the arduino read from serial
    //paddleMotorStopped = 1;
    //paddleMotor(paddleMotorStopped, paddleFastSection);
    // Get serial input
    String trashStr = Serial.readStringUntil('\n');
    trashGet = trashStr.toInt();
  }
  return trashGet;  // Return serial input
}
// Send the feedback to Rpi4
void sendFeedbackToPi(int feedbackNumber){
  Serial.println(feedbackNumber);
  delay(20);
}