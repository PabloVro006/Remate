// RELAY SETUP
#define CLOCK_DISK_RELAY 2
#define COUNTER_DISK_RELAY 4
#define CLOCK_CROSS_RELAY 7
#define COUNTER_CROSS_RELAY 8
#define PADDLE_RELAY 12

// ENUM FOR TRASH TYPES
enum TrashType {
  TRASH_NONE = 0,
  TRASH_METAL = 1,
  TRASH_PLASTIC = 2,
};

// HALL SETUP
int HALL_DISK = A0;
int HALL_CROSS = A1;

// CONSTANTS
const int serialDelay = 20;
const int hallThresholdLow = 400;
const int hallThresholdHigh = 550;
const int rotationDelayMs = 1000;
const int feedbackOk = 42;

// Assign variables to represent specific trash types
const TrashType trashTypeMetal = TRASH_METAL;
const TrashType trashTypePlastic = TRASH_PLASTIC;

// Logic for the paddle motor going delay
unsigned long previousMillis = 0;    // Stores the last time the action was taken
const long paddleGoingInterval = 200;
const long paddleNotGoingInterval = 600;

// TRASHING SETUP
bool isRotating = false;
int trash = TRASH_NONE;

// MOTOR STRUCT
typedef struct {
  int COUNTER_RELAY;
  int CLOCK_RELAY;
  int HALL;
} MotorData;
static const MotorData motorData[] = {
  {COUNTER_DISK_RELAY, CLOCK_DISK_RELAY, HALL_DISK},
  {COUNTER_CROSS_RELAY, CLOCK_CROSS_RELAY, HALL_CROSS}
};

typedef struct {
  bool power;
  bool going;
} PaddleMotorStruct;
PaddleMotorStruct paddleMotorStruct;

// DECLEARING FUNCTIONS
bool hallCheck(int hall);                                                                            // Check for disk or cross hall
void turnMotorsOff(const int motorIndexes[]);                                                        // Turn off motor passed in the array
void controlPaddleMotor(PaddleMotorStruct* motorController);                                         // Control paddle motor
void rotateMotor(uint8_t motorIndex, uint8_t direction, uint8_t times);                              // Rotate cross or disk
void throwTrash(TrashType trashType);                                                                // Throw trash
int getTrashFromPi();                                                                                // Get serial input from Rpi4
void sendFeedbackToPi(int feedbackNumber);                                                           // Send the feedback to Rpi4

// SETUP
void setup() {
  // SERIAL INITIALIZATION
  delay(1000);
  Serial.begin(115200);
  delay(serialDelay);

  // HALL INITIALIZATION
  pinMode(HALL_DISK, INPUT);
  pinMode(HALL_CROSS, INPUT);

  // RELAY INITIALIZATION
  pinMode(PADDLE_RELAY, OUTPUT);
  for (int i = 0; i < 2; i++) {
    pinMode(motorData[i].COUNTER_RELAY, OUTPUT);
    pinMode(motorData[i].CLOCK_RELAY, OUTPUT);
    digitalWrite(motorData[i].COUNTER_RELAY, HIGH);
    digitalWrite(motorData[i].CLOCK_RELAY, HIGH);
  }
  paddleMotorStruct.power = 1;
  paddleMotorStruct.going = 1;

  // CALIBRATION
  //rotateMotor(0, 1, 4); // Disk calibration
  rotateMotor(1, 0, 1); // Cross calibration
}

// LOOP
void loop() {
  // Get current time
  unsigned long currentMillis = millis();
  unsigned long intervalToConsider = paddleMotorStruct.going ? paddleGoingInterval : paddleNotGoingInterval;
  if (currentMillis - previousMillis >= intervalToConsider) {
    previousMillis = currentMillis;
    paddleMotorStruct.going = !paddleMotorStruct.going;
  }

  // Get trash
  if(!isRotating){
    trash = getTrashFromPi();
    if(trash != TRASH_NONE){
      paddleMotorStruct.power = 0;
      controlPaddleMotor(&paddleMotorStruct);
      isRotating = true;
      trash == TRASH_METAL ? throwTrash(trashTypeMetal) : throwTrash(trashTypePlastic);
      sendFeedbackToPi(feedbackOk);
      paddleMotorStruct.power = 1;
      paddleMotorStruct.going = 0;
    }
  }
  controlPaddleMotor(&paddleMotorStruct);
}

// DEFINE FUNCTIONS
// Hall reading function
bool hallCheck(int hall) {
  delay(serialDelay);
  int reading = analogRead(hall);
  return (reading > hallThresholdLow && reading < hallThresholdHigh);
}

// Turn off motors
void turnMotorsOff(const int motorIndexes[]){
	for (int i=0; motorIndexes[i] != -1; i++){
    digitalWrite(motorData[motorIndexes[i]].COUNTER_RELAY, HIGH);
    digitalWrite(motorData[motorIndexes[i]].CLOCK_RELAY, HIGH);
	}
}

// Paddle movement
void controlPaddleMotor(PaddleMotorStruct* motorController) {
  if(motorController->power && motorController->going) {
    digitalWrite(PADDLE_RELAY, HIGH);
  }else{
    digitalWrite(PADDLE_RELAY, LOW);
  }
}

// Generic function to move motors
void rotateMotor(uint8_t motorIndex, uint8_t rotationDirection, uint8_t times) {
  int motors[2] = {(int)motorIndex, -1};
  for (int i = 0; i < times; i++) {
    digitalWrite(motorData[motorIndex].COUNTER_RELAY, rotationDirection);
    digitalWrite(motorData[motorIndex].CLOCK_RELAY, !rotationDirection);
    delay(rotationDelayMs);
    while (hallCheck(motorData[motorIndex].HALL)) {
      digitalWrite(motorData[motorIndex].COUNTER_RELAY, rotationDirection);
      digitalWrite(motorData[motorIndex].CLOCK_RELAY, !rotationDirection);
    }
    turnMotorsOff(motors);
  }
}

// Throw function
void throwTrash(TrashType trashType){
  uint8_t motorIndex = (trashType == TRASH_METAL) ? 0 : 1;
  //uint8_t order = (trashType == TRASH_METAL) ? 1 : 0; // 0 for counterclockwise, 1 for clockwise
  rotateMotor(motorIndex,0,1);
	rotateMotor(motorIndex,1,1);
  trash = TRASH_NONE;
}

// RPI4 COMMUNICATION
// Get trash from Rpi4
int getTrashFromPi() {  
  int trashGet = TRASH_NONE;
  if (Serial.available() > 0) {
    // Get serial input
    trashGet = Serial.parseInt();
    if (trashGet < TRASH_NONE || trashGet > TRASH_PLASTIC) {
      trashGet = TRASH_NONE;  // Reset to default if invalid
    }
    Serial.flush();
  }
  return trashGet;  // Return serial input
}
// Send the feedback to Rpi4
void sendFeedbackToPi(int feedbackNumber){
  Serial.println(feedbackNumber);
  delay(serialDelay);
  Serial.flush();
  isRotating = false;
}