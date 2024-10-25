// RELAY SETUP
#define COUNTER_DISK_RELAY 2
#define CLOCK_DISK_RELAY 4
#define COUNTER_CROSS_RELAY 8
#define CLOCK_CROSS_RELAY 7
#define PADDLE_RELAY 12

// HALL SETUP
int hallDisk = A0;
int hallCross = A1;

// PADDLE SETUP
int paddleMotorState = 1;

// CONSTANTS
const int serialDelay = 20;
const int hallThresholdLow = 490;
const int hallThresholdHigh = 550;
const int rotationDelayMs = 900;
const int feedbackOk = 42;

// ENUM FOR TRASH TYPES
enum TrashType {
  TRASH_NONE = 0,
  TRASH_PLASTIC = 1,
  TRASH_PAPER = 2,
  TRASH_METAL = 3,
  TRASH_NR = 4,
  EMPTY = 5
};

// TRASHING SETUP
int diskState[4] = {TRASH_NONE, TRASH_NONE, TRASH_NONE, EMPTY};  // empty, empty, empty, hole
bool isRotating = false;
int trash = TRASH_NONE;

// MOTOR STRUCT
typedef struct {
  int COUNTER_RELAY;
  int CLOCK_RELAY;
  int HALL;
} MotorData;

static const MotorData motorData[] = {
  {COUNTER_DISK_RELAY, CLOCK_DISK_RELAY, hallDisk},
  {COUNTER_CROSS_RELAY, CLOCK_CROSS_RELAY, hallCross}
};

// DECLEARING FUNCTIONS
bool hallCheck(int hall);                                                                            // Check for disk or cross hall
void turnMotorsOff(const int motorIndexes[]);                                                        // Turn off motor passed in the array
void controlPaddleMotor(int motorController);                                                        // Control paddle motor
void rotateMotor(uint8_t motorIndex, uint8_t direction, uint8_t times);                              // Rotate cross or disk
void throwPaper();                                                                                   // Disk rotate 2 times (counter clock) and goes back
void throwPaperPlastic();                                                                            // Disk do a full rotation (clock wise)
void normalThrow(uint8_t motorIndex, uint8_t diskSpace, uint8_t order);                              // Throw plastic or nr
void rotateList();                                                                                   // Make element in diskState rotate
int getTrashFromPi();                                                                                // Get serial input from Rpi4
void sendFeedbackToPi(int feedbackNumber);                                                           // Send the feedback to Rpi4

// SETUP
void setup() {
  delay(1000); // Wait a bit

  // SERIAL INITIALIZATION
  Serial.begin(115200);

  // HALL INITIALIZATION
  pinMode(hallDisk, INPUT);
  pinMode(hallCross, INPUT);

  // RELAY INITIALIZATION
  int relays[] = {COUNTER_DISK_RELAY, CLOCK_DISK_RELAY, COUNTER_CROSS_RELAY, CLOCK_CROSS_RELAY, PADDLE_RELAY, -1};
  for (int i = 0; relays[i] != -1; i++) {
    pinMode(relays[i], OUTPUT);
    digitalWrite(relays[i], HIGH);  // Set to off
  }

  // CALIBRATION
  // Disk calibration
  //throwPaperPlasticCombo();
  // Cross calibration
  rotateMotor(1, 1, 1);
}

// LOOP
void loop() {
  // CONTROL PADDLE
  // Set motor state (off/on)
  paddleMotorState = (diskState[0] == TRASH_NONE) ? 1 : 0;

  // Get trash
  if(!isRotating){
    trash = getTrashFromPi();
    // Now control the paddle movement
    if (trash != TRASH_NONE) {
      controlPaddleMotor(0);
      diskState[0] = trash;
    } else {
      controlPaddleMotor(paddleMotorState);
    }
  }

  // INITIAL CHECK
  // Check for paper + plastic combo
  if (diskState[1] == TRASH_PAPER && diskState[0] == TRASH_PLASTIC) {
    isRotating = true;
    controlPaddleMotor(0);
    throwPaperPlastic();
    controlPaddleMotor(paddleMotorState);
    sendFeedbackToPi(feedbackOk);
    isRotating = false;
  }
  // Check for metal + paper combo
  else if (diskState[2] == TRASH_METAL && diskState[1] == TRASH_PAPER) {
    isRotating = true;
    controlPaddleMotor(0);
    throwPaper();
    controlPaddleMotor(paddleMotorState);
    sendFeedbackToPi(feedbackOk);
    isRotating = false;
  }
  // Check for metal
  else if (diskState[2] == TRASH_METAL) {
    isRotating = true;
    controlPaddleMotor(0);
    normalThrow(0, 2, 0);
    controlPaddleMotor(paddleMotorState);
    sendFeedbackToPi(feedbackOk);
    isRotating = false;
  }
  // Check for paper
  else if (diskState[1] == TRASH_PAPER) {
    isRotating = true;
    controlPaddleMotor(0);
    throwPaper();
    controlPaddleMotor(paddleMotorState);
    sendFeedbackToPi(feedbackOk);
    isRotating = false;
  }

  // NORMAL CASE
  switch (trash) {
    case TRASH_PLASTIC:
      isRotating = true;
      normalThrow(0, 0, 1);
      sendFeedbackToPi(feedbackOk);
      isRotating = false;
      break;
    case TRASH_PAPER:
      isRotating = true;
      rotateMotor(1, 1, 1);
      sendFeedbackToPi(feedbackOk);
      rotateList();
      isRotating = false;
      break;
    case TRASH_METAL:
      isRotating = true;
      rotateMotor(1, 1, 1);
      sendFeedbackToPi(feedbackOk);
      rotateList();
      isRotating = false;
      break;
    case TRASH_NR:
      isRotating = true;
      normalThrow(1, 0, 0);
      sendFeedbackToPi(feedbackOk);
      isRotating = false;
      break;
    default:
      delay(10);
  }
}

// DEFINE FUNCTIONS
// Hall reading function
bool hallCheck(int hall) {
  int reading = analogRead(hall);
  return (reading > hallThresholdLow && reading < hallThresholdHigh);
}

// Turn off motor
void turnMotorsOff(const int motorIndexes[]){
	for (int i=0; motorIndexes[i] != -1; i++){
    digitalWrite(motorData[i].COUNTER_RELAY, HIGH);
    digitalWrite(motorData[i].CLOCK_RELAY, HIGH);
	}
}

// Paddle movement
void controlPaddleMotor(int motorController) {
  digitalWrite(PADDLE_RELAY, motorController);
}

// Disk & cross rotation
void rotateMotor(const uint8_t motorIndex, const uint8_t rotationDirection, const uint8_t times) {
  int motors[2] = {(int)motorIndex, -1};
  for (int i = 0; i < times; i++) {
    digitalWrite(motorData[motorIndex].COUNTER_RELAY, !rotationDirection);
    digitalWrite(motorData[motorIndex].CLOCK_RELAY, rotationDirection);
    delay(rotationDelayMs);
    while (hallCheck(motorData[motorIndex].HALL)) {
      digitalWrite(motorData[motorIndex].COUNTER_RELAY, !rotationDirection);
      digitalWrite(motorData[motorIndex].CLOCK_RELAY, rotationDirection);
    }
    turnMotorsOff(motors);
  }
}

// THROWING FUNCTION
// Disk rotate 2 times counter wise then goes back
void throwPaper(){
	rotateMotor(0,0,2);
	rotateMotor(0,1,2);
	diskState[2] = diskState[1] = TRASH_NONE;
}
// Disk rotate 4 times clock wise
void throwPaperPlastic() {
  rotateMotor(0, 1, 4);
  memset(diskState, TRASH_NONE, 3 * sizeof(int));
  diskState[3] = EMPTY;
}

// Generic function for throw both plastic & nr
void normalThrow(const uint8_t motorIndex, const uint8_t diskSpace, const uint8_t order){
	rotateMotor(motorIndex,order,1);
	rotateMotor(motorIndex,!order,1);
	diskState[diskSpace] = TRASH_NONE;
}

// Rotate element in diskState[]
void rotateList() {
  diskState[2] = diskState[1];
  diskState[1] = diskState[0];
  diskState[0] = TRASH_NONE;
}

// RPI4 COMMUNICATION
// Get trash from Rpi4
int getTrashFromPi() {
  int trashGet = TRASH_NONE;
  if (Serial.available() > 0) {
    // Turn off paddle motor until the arduino read from serial
    //paddleMotorStopped = 1;
    // Get serial input
    String trashStr = Serial.readStringUntil('\n');
    trashGet = trashStr.toInt();
    if (trashGet < TRASH_NONE || trashGet > TRASH_NR) {
      trashGet = TRASH_NONE;  // Reset to default if invalid
    }
  }
  return trashGet;  // Return serial input
}
// Send the feedback to Rpi4
void sendFeedbackToPi(int feedbackNumber){
  Serial.println(feedbackNumber);
  delay(serialDelay);
}