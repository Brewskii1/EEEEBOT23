#include <ESP32Encoder.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <Wire.h>

#define enA 33  //EnableA command line - should be a PWM pin
#define enB 25   //EnableB command line - should be a PWM pin

//name the motor control pins - replace the CHANGEME with your pin number, digital pins do not need the 'D' prefix whereas analogue pins need the 'A' prefix
#define INa 26  //Channel A direction 
#define INb 27  //Channel A direction 
#define INc 14  //Channel B direction 
#define INd 12  //Channel B direction 

byte speedSetting = 170;   //initial speed = 0

// setting PWM properties
const int freq = 2000;
const int servoFrequency = 50;
const int ledChannela = 0;
const int ledChannelb = 1;
const int servoChannel = 2;
const int resolution = 8;
const int servoResolution = 12;

//Servo Value recordings
//Middle = 310
//Max right = 520
//Max Left = 100
int servoPin = 13;
float steeringAngle;  // variable to store the servo position

ESP32Encoder encoder1;
ESP32Encoder encoder2;

// timer and flag for example, not needed for encoders
unsigned long encoder2lastToggled;
bool encoder2Paused = false;

//Encoder position
float newPosition;
//what step of parking is completed
int parked = 0;
//I2C Message
char message;
char encoderBuffer[7];

//encodes 0-255 to 100-520 range that was found for the servo.
void changeSteeringAngle(float angle){
  steeringAngle = (angle * 1.65) + 100;
  ledcWrite(servoChannel, steeringAngle);
    // Serial.print("Steering Angle: ");
    // Serial.println(abs(steeringAngle));
}

void wiggle(int wiggleno){
  for (int i=0; i < wiggleno; i++){
    changeSteeringAngle(0);
    delay(100);
    changeSteeringAngle(255);
    delay(100);
    Serial.println(i);
    Serial.print("Wiggleno: ");
    Serial.println(wiggleno);
  }
}

void straight(){
  changeSteeringAngle(126);
}


void checkEncoders(){
  //ENCODER CODE
  newPosition = ((encoder1.getCount() / 2)+(encoder2.getCount() / 2))/2 ;
  // Serial.println(newPosition);
}

void checkEncoder1(){
  //ENCODER CODE
  newPosition = (encoder1.getCount() / 2);
  Serial.println(newPosition);
}

void checkEncoder2(){
  //ENCODER CODE
  newPosition = (encoder2.getCount() / 2);
  Serial.println(newPosition);
}



// Remember that the checkEncoders is an average between the two, so distance can be different give a non centred eDifferential.
void goDistance(float distance){ 
  checkEncoders();
  float tempPosition = newPosition;
  while (abs(newPosition - tempPosition) < distance){
    checkEncoders();
    Serial.print("\nTarget Position: ");
    Serial.print(tempPosition);
    Serial.print("\nCurrent Position: ");
    Serial.print(newPosition);
    delay(50);
  }
}

void edifferential(float angle){ // Takes value between 0-1 and translates that to ratio of left motor speed to right motor speed.
  float leftspeed, rightspeed;
  if (angle < 0.5){
    leftspeed = angle * speedSetting * 2;
    rightspeed = speedSetting;
  } else {
    rightspeed = (1 - angle) * speedSetting * 2;
    leftspeed = speedSetting; 
  }
  motors(leftspeed, rightspeed);
}

void motors(int leftSpeed, int rightSpeed) {
  //set individual motor speed
  //direction is set separately
  ledcWrite(ledChannela, leftSpeed);
  ledcWrite(ledChannelb, rightSpeed);
  delay(25);
}

void goForwards() {
  digitalWrite(INa, HIGH);
  digitalWrite(INb, LOW);
  digitalWrite(INc, HIGH);
  digitalWrite(INd, LOW);
}

void goBackwards() {
  digitalWrite(INa, LOW);
  digitalWrite(INb, HIGH);
  digitalWrite(INc, LOW);
  digitalWrite(INd, HIGH);
}

void stopMotors() {
  digitalWrite(INa, LOW);
  digitalWrite(INb, LOW);
  digitalWrite(INc, LOW);
  digitalWrite(INd, LOW);
}

// input steering angle manually through the serial monitor.
void inputSteeringAngle(){
  steeringAngle = Serial.parseInt();
  if (steeringAngle >1){
    changeSteeringAngle(steeringAngle);
  }
}

// events to be made upon different requests of the Master ESP32
void eventAct(){
  switch(message)
  {
    case 's':
      break; // REMOVE FUNCTIONALITY
      Serial.println("STOP RECIEVED");
      if (parked == 0){
      park();
      } else {
        stopMotors();
        Serial.println("DONE");
        changeSteeringAngle(127);
        while (true){
        }
      }
      break;
    case 'w':
      Serial.print("WIGGLING");
      delay(50);
      wiggle(1);
      delay(50);
      message = ' ';
      break;
      
  }
}

void receiveEvent(int numBytes){
  Serial.println("\nMessage Recieved");
  message = ' ';
  while (Wire.available()) {
    char c = Wire.read();
    message = message + c;
  Serial.println(message);    // write string to serial monitor
  delay(100);
  } 
}

void requestEvent() { // Writing Encoder values to the master ESP32.
  Serial.println("request evented");
  checkEncoders();
  dtostrf(newPosition, 7, 2, encoderBuffer);
  Wire.write(encoderBuffer);
}

// Series of functions to produce the parking feature.
void park(){ 
  Serial.print("parking");
  goForwards();
  goDistance(20);
  stopMotors();
  delay(200);
  changeSteeringAngle(255);
  edifferential(0.8);
  goBackwards();
  goDistance(40);
  edifferential(0.5);
  straight();
  message = 'g';
  parked = 1;
}

void setup() {
  // put your setup code here, to run once:
  pinMode(INa, OUTPUT);
  pinMode(INb, OUTPUT);
  pinMode(INc, OUTPUT);
  pinMode(INd, OUTPUT);
  // pinMode(enA, OUTPUT);
  // pinMode(enB, OUTPUT);  //if defining some pins as PWM, DON'T set them as OUTPUT!

  // configure LED PWM functionalitites
  ledcSetup(ledChannela, freq, resolution);
  ledcSetup(ledChannelb, freq, resolution);
  ledcSetup(servoChannel, servoFrequency, servoResolution); //servo setup on PWM2, 50Hz, 12-bit (0-4096)

  //attach the channel to the GPIO to be controlled

  ledcAttachPin(enA, ledChannela);
  ledcAttachPin(enB, ledChannelb);
  ledcAttachPin(servoPin, servoChannel);

  //Setup I2C with other ESP32
  Wire.begin(0x08);             // join i2c bus with address 8
  Wire.onReceive(receiveEvent); // create a receive event
  Wire.onRequest(requestEvent); // create a request event

  //initialise Serial Communication
  Serial.begin(9600);
  Serial.println("ESP32 Running");  //sanity Check
  encoder1.attachHalfQuad ( 36, 39 );
  encoder1.setCount ( 0 );
  encoder2.attachHalfQuad ( 34, 35 );
  encoder2.setCount ( 0 );
  goBackwards();
  //setup the steering angle
  straight();
  delay(100);
  wiggle(3);
  delay(1500);
  motors(speedSetting, speedSetting);  //make a call to the 'motors' function and provide it with a value for each of the 2 motors - can be different for each motor - using same value here for expedience
  Serial.println("Starting...");
  straight();
}

/*void bomb(){
  speedSetting = 20;
  for (int i = 500; i)
}*/

void loop() {
  eventAct();
  checkEncoders();
  delay(100);
}





