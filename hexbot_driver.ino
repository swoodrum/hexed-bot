#include <SoftwareSerial.h>

// NOTE: digital pins 2, 6, 7, 8, and optionally 9 are used by the video shield.
// the video shield also uses analog pin 2

#define pRxPin 3 // for Pololu servo controller, digital pin 3
#define pTxPin 4 // for Pololu servo controller, digital pin 4

const int gp2d12Pin = 5; // analog pin 5

SoftwareSerial mySerial(pRxPin, pTxPin);

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  delay(1000);
  center_servos();
}

void loop() {
  //Serial.println(read_gp2d12_range(gp2d12Pin));
  if(read_gp2d12_range(gp2d12Pin) > 30) {
    walk_forward(1); 
  } 
  else {
    walk_backward(1); 
  }
  sweep();
  pan();
  delay(5000);
}

/* 
 read_gp2d12_range
 Function that reads a value from GP2D12 infrared distance sensor and returns a value in centimeters.
 This sensor should be used with a refresh rate of 36ms or greater.
 Javier Valencia 2008
 float read_gp2d12_range(byte pin)
 It can return -1 if something gone wrong.
 */

float read_gp2d12_range(byte pin) {
  int tmp;
  tmp = analogRead(pin);
  if (tmp < 3)
    return -1; // invalid value
  return (6787.0 /((float)tmp - 3.0)) - 4.0;
}

// We are using the Pololu serial command protocol
// FYI see http://www.pololu.com/docs/0J40/5.e
// The servo target values are specific to my servos

void move_servo(int channel, int pos) {
  byte command[] = {
    170, 12, 4, channel, pos & 0x7F, (pos >> 7) & 0x7F };
  for(int i = 0; i < 6; i++) {
    mySerial.write(command[i]);
    //Serial.println(command[i],HEX);
  } 
}

// move camera left right
void sweep() {
  int pos = 6000;
  move_servo(1, pos);
  //sweep left
  for(pos; pos < 8000; pos += 50) {
    move_servo(1, pos);
    delay(10); 
  }
  delay(200);
  //sweep right
  for(pos; pos > 3968; pos -= 50) {
    move_servo(1, pos);
    delay(10); 
  }
  delay(200);
  // move back to center
  for(pos; pos < 6000; pos += 50) {
    move_servo(1, pos);
    delay(10);
  }
}

// move camera head up/down
void pan() {
  int pos = 4632;
  move_servo(0, pos);
  // look up
  for(pos; pos < 6360; pos += 50) {
    move_servo(0, pos);
    delay(10);
  }
  delay(200);
  // look down
  for(pos; pos > 2752; pos -= 50) {
    move_servo(0, pos);
    delay(10);
  }
  delay(200);
  // move to level
  for(pos; pos < 4632; pos += 50) {
    move_servo(0, pos);
    delay(10); 
  }
}

void center_servos() {
  move_servo(4, 5380);
  move_servo(3, 4887);
  move_servo(2, 6000);
  move_servo(1, 6000);
  move_servo(0, 4632); 
}



void mid_left_down() { 
  move_servo(2, 7217);
}

void mid_right_down() { 
  move_servo(2, 3968);
}

void right_legs_backward() { 
  move_servo(3, 3987); 
}

void right_legs_forward() {
  move_servo(3, 5846);  
}

void left_legs_forward() { 
  move_servo(4, 4653);
}

void left_legs_backward() { 
  move_servo(4, 6336);
}

void walk_forward(int steps) {
  for(int i = 0; i < steps; i++) {
    mid_left_down();
    delay(150);
    right_legs_backward();
    delay(150);
    left_legs_forward();
    delay(150);
    mid_right_down();
    delay(150);
    left_legs_backward();
    delay(150);
    right_legs_forward();
    delay(150);
    center_servos();
  }
}

void walk_backward(int steps) {
  for(int i = 0; i < steps; i++) {
    mid_left_down();
    delay(150);
    right_legs_forward();
    delay(150);
    left_legs_backward();
    delay(150);
    mid_right_down();
    delay(150);
    left_legs_forward();
    delay(150);
    right_legs_backward();
    delay(150);
    mid_left_down();
    delay(150);
    right_legs_forward();
    delay(150);
    center_servos();
  }
}

void turn_right(int numTurns) {
  for(int i = 0; i < numTurns; i++) {
    mid_left_down();
    delay(150);
    left_legs_forward();
    delay(150);
    mid_right_down();
    delay(150);
    left_legs_backward();
    delay(150);
  }
  center_servos();
}

void turn_left(int numTurns) {
  for(int i = 0; i < numTurns; i++) {
    mid_right_down();
    delay(150);
    right_legs_forward();
    delay(150);
    mid_left_down();
    delay(150);
    right_legs_backward();
    delay(150);
  }
  center_servos();
}
























