#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <stdlib.h>
#include <stdio.h>

const int trigPin = 5;  
const int echoPin = 18  ; 

float duration, distance;  

String message;
float encoderDist;

long lastMsg = 0;
char msg[50];
int value = 0;

const int ledPin = 4;


// PID Constants
float Kp = 0.5; // Proportional gain
float Ki = 0.1; // Integral gain
float Kd = 0.2; // Derivative gain

float previousError = 0.0;
float integral = 0.0;

void setup() {
  pinMode(trigPin, OUTPUT);  
  pinMode(echoPin, INPUT);  

  //Each IR sensor analog input
  pinMode(32, INPUT);
  pinMode(33, INPUT);
  pinMode(25, INPUT);
  pinMode(26, INPUT);
  pinMode(27, INPUT);
  pinMode(14, INPUT);

  Serial.begin(9600);
  Serial.print("STARTING");  

  Wire.begin();             // join i2c bus with address 8
  delay(2000);  
}

void loop() {
  int angle = PID();
  sendAngle(angle);
  // //HC-SR04 Write
  // digitalWrite(trigPin, LOW);  
  // delayMicroseconds(2);  
  // digitalWrite(trigPin, HIGH);  
  // delayMicroseconds(10);  
  // digitalWrite(trigPin, LOW);  

  // //HC-SR04 Read
  // sensorRead();
  // stopCheck();
  delay(100);  
}

void stopCheck(){
  if (distance <=10){
    Wire.beginTransmission(0x08); // transmit to slave device address 8
    Wire.write("S");
    Wire.endTransmission();       // end transmission
    Serial.println("Sending STOP request");
  }
}

void sendAngle(int x){
  Wire.beginTransmission(0x08); // transmit to device #4
  Wire.write((byte)((x & 0x0000FF00) >> 8));    // first byte of x, containing bits 16 to 9
  Wire.write((byte)(x & 0x000000FF));           // second byte of x, containing the 8 LSB - bits 8 to 1
  Wire.endTransmission();   // stop transmitting
  }

float PID() {
  //Taking inputs from GPIO pins
  // int val1 = analogRead(32);
  int val2 = analogRead(33);
  // int val3 = analogRead(25);
  int val4 = analogRead(26);
  // int val5 = analogRead(27);
  int val6 = analogRead(14);

  int IRarray[3] = {val2, val4, val6};
  float weightedAverage = 0;
  
  // Calculate weighted average
  for (int i = 0; i < 3; i++) {
    weightedAverage += ((i - 1) * IRarray[i]); 
  }
  

  
  // threshold value for considering black color
  int targetValue = 3000;
  
  // calculate the error as the difference between the target and the weighted average
  float error = targetValue - weightedAverage;
  
  // calculating all terms
  float proportional = Kp * error;
  integral += Ki * error;
  float derivative = Kd * (error - previousError);

  // combine all terms and keep output within servo range
  float output = proportional + integral + derivative;
  int angle = constrain(output, 45, 135);

  // Update the previous error for the next iteration
  previousError = error;

  Serial.println(angle);
  return angle;
}

//      Max(White) Min(black)
// val1 4095       4095
// val2 4095       1100
// val3 4095       4095
// val4 4095       1400
// val5 4095       4095
// val6 3600       1030


// VAL1 3, 5 dont work :(
