// TRANSISTOR SETUP
// Setup for the disk's motor
#define CLOCK_DISK_PNP 2
#define COUNTER_DISK_PNP 3
#define CLOCK_DISK_NPN 4
#define COUNTER_DISK_NPN 5
// Setup for the cross's motor
#define CLOCK_CROSS_PNP 6
#define COUNTER_CROSS_PNP 7
#define CLOCK_CROSS_NPN 8
#define COUNTER_CROSS_NPN 9
// Setup for the paddle motor
#define PADDLE_NPN 12

// ENUM FOR TRASH TYPES
enum TrashType {
  TRASH_NONE = 0,
  TRASH_METAL = 1,
  TRASH_PLASTIC = 2,
  TRASH_INCOMING = 9,
};

// HALL SETUP
const int HALL_DISK = A0;
const int HALL_CROSS = A1;

// CONSTANTS
const int serialDelay = 20;
const int hallThresholdLow = 400;  // If hall's value < than this, there is a magnet 
const int hallThresholdHigh = 550; // If hall's value > thas this, there is amagnet
const int rotationDelayMs = 1000;  // Millis for making sure that the magnet has moved away from the hall
const int feedbackOk = 42;         // Number to send at the Rpi4 when throwing is over
// Assign variables to represent specific trash types
const TrashType trashTypeMetal = TRASH_METAL;
const TrashType trashTypePlastic = TRASH_PLASTIC;
// Logic for the paddle motor going delay
const long paddleGoingInterval = 200;    // Millis indicating the time of paddle's going
const long paddleNotGoingInterval = 600; // Millis indicating the time od paddle's stopping
const long trashIncomingTimeout = 5000;  // Millis for exiting the 9 condition if nothing is received
unsigned long previousMillis = 0;        // Stores the last time the switch of the paddle's going was changed

// TRASHING SETUP
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
void controlPaddleMotor(PaddleMotorStruct* motorController);             // Control paddle motor
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
  //rotateMotor(0, 1, 4); // Disk calibration
  rotateMotor(1, 0, 1); // Cross calibration
}

// LOOP
void loop() {
  // Get current time
  unsigned long currentMillis = millis();
  //unsigned long intervalToConsider = paddleMotorStruct.going ? paddleGoingInterval : paddleNotGoingInterval;
  if (currentMillis - previousMillis >= (paddleMotorStruct.going ? paddleGoingInterval : paddleNotGoingInterval)) {
    previousMillis = currentMillis;
    paddleMotorStruct.going = !paddleMotorStruct.going;
  }

  // Get trash
  if(!isThrowing){
    trash = getTrashFromPi();
    if(trash != TRASH_NONE){
      paddleMotorStruct.power = 0;
      controlPaddleMotor(&paddleMotorStruct);
      if(trash == TRASH_INCOMING){
        unsigned long startMillis = millis();
        while(trash == TRASH_INCOMING || trash == TRASH_NONE){
          unsigned long currentMillisForTrashIncoming = millis();
          trash = getTrashFromPi();
          delay(serialDelay);
          if(currentMillisForTrashIncoming - startMillis >= trashIncomingTimeout){
            trash = TRASH_NONE;
            paddleMotorStruct.power = 1;
            return; // Restarts the loop()
          }
        };
      }
      isThrowing = true;
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
    const MotorData& motor = motorData[motorIndexes[i]];
    digitalWrite(motor.CLOCK_NPN, LOW);
    digitalWrite(motor.CLOCK_PNP, LOW);
    digitalWrite(motor.COUNTER_NPN, LOW);
    digitalWrite(motor.COUNTER_PNP, LOW);
	}
}

// Paddle movement
void controlPaddleMotor(PaddleMotorStruct* motorController) {
  if(motorController->power && motorController->going) {
    digitalWrite(PADDLE_NPN, HIGH);
  }else{
    digitalWrite(PADDLE_NPN, LOW);
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
  uint8_t order = (trashType == TRASH_METAL) ? 1 : 0; // 0 for clockwise, 1 for counterclockwise
  rotateMotor(motorIndex,order,1);
  if(trashType == TRASH_METAL){
    rotateMotor(motorIndex,!order,1);
  }
  trash = TRASH_NONE;
}

// RPI4 COMMUNICATION
// Get trash from Rpi4
int getTrashFromPi() {  
  Serial.flush();
  int trashGet = TRASH_NONE;
  if (Serial.available() > 0) {
    // Get serial input
    trashGet = Serial.parseInt();
    if ((trashGet < TRASH_NONE || trashGet > TRASH_PLASTIC) && trashGet != TRASH_INCOMING) {
      trashGet = TRASH_NONE;  // Reset to default if invalid
    }
  }
  return trashGet;  // Return serial input
}
// Send the feedback to Rpi4
void sendFeedbackToPi(int feedbackNumber){
  Serial.println(feedbackNumber);
  Serial.flush();
  delay(serialDelay);
  while (Serial.available() > 0) { // Clear any remaining data in the input buffer
    Serial.read();
  }
  isThrowing = false;
}