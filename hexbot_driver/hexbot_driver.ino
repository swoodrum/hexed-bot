#include <SoftwareSerial.h>
#include <TVout.h>
#include <fontALL.h>
#define W 128
#define H 96

// NOTE: digital pins 2, 6, 7, 8, and optionally 9 are used by the video shield.
// the video shield also uses analog pin 2

#define pRxPin 3 // for Pololu servo controller, digital pin 3
#define pTxPin 4 // for Pololu servo controller, digital pin 4
#define SET_TARGET_BYTE 4 // represents Pololu protocol set target command
#define DEFAULT_BAUD 9600
#define DEVICE_NUM 12 // default device number for Pololu protocol
#define MIN_RANGE 30
#define PROTOCOL_BYTE 170 // Pololu protocol identifier (0xAA)
#define SERVO_0_NEUTRAL 4632
#define SERVO_0_MAX 6360
#define SERVO_0_MIN 2752
#define SERVO_1_NEUTRAL 6000
#define SERVO_1_MAX 8000
#define SERVO_1_MIN 3968
#define SERVO_2_NEUTRAL 6000
#define SERVO_3_NEUTRAL 4887
#define SERVO_4_NEUTRAL 5380

const int gp2d12Pin = 5; // analog pin 5

TVout tv;

SoftwareSerial mySerial(pRxPin, pTxPin);

unsigned char c;
unsigned char minX,minY,maxX,maxY;
//char s[32];

int servoX = SERVO_1_NEUTRAL;

void setup() {
  tv.begin(NTSC, W, H);
  initOverlay();
  initInputProcessing();
  Serial.begin(DEFAULT_BAUD);
  mySerial.begin(DEFAULT_BAUD);
  delay(1000);
  center_servos();
  
}

void loop() {
  //Serial.println(read_gp2d12_range(gp2d12Pin));
  /*if(read_gp2d12_range(gp2d12Pin) > MIN_RANGE) {
   walk_forward(1); 
   } 
   else {
   walk_backward(1); 
   }*/
  //sweep();
  //pan();
  tv.capture();
  minX = W;
  minY = H;
  maxX = 0;
  maxY = 0;
  boolean found = 0;
  for(int y=0;y<H;y++) {
    for(int x=0;x<W;x++) {
      c = tv.get_pixel(x,y);
      if (c == 1) {
        found = true;
        if (x < minX) {
          minX = x;
        }
        if (x > maxX) {
          maxX = x;
        }
        if (y < minY) {
          minY = y;
        }
        if (y > maxY) {
          maxY = y;
        }
      }
    }
  }
  if(found) {
    /*Serial.print("minX: ");
     Serial.print(minX);
     Serial.print("\tmaxX: ");
     Serial.print(maxX);
     Serial.print("\tminY: ");
     Serial.print(minY);
     Serial.print("\tmaxY: ");
     Serial.println(maxY);*/
    int rawX = (maxX+minX)/2;
    //int translateX = map(rawX, 0, W, SERVO_1_MAX, SERVO_1_MIN);
    int translateX = map(rawX, 0, W, SERVO_1_MAX, SERVO_1_MIN);
    //float translateX = map(rawX, 0, W, 0, 1);
    
    //int rawY = (maxY+minY)/2;
    //int translateY = map(rawY, 0, H, SERVO_0_MIN, SERVO_0_MAX);
    //sprintf(s, "%d, %d", ((maxX+minX)/2), ((maxY+minY)/2));
    //sprintf(s, "%d, %d", rawX, rawY);
    Serial.println(translateX);
    if(translateX > SERVO_1_NEUTRAL) {
      servoX = min(SERVO_1_MAX,(servoX + (translateX - SERVO_1_NEUTRAL)));
    } else if (translateX < 6000) {
      servoX = max(SERVO_1_MIN, (servoX - (SERVO_1_NEUTRAL - translateX))); 
    } else {
      servoX = SERVO_1_NEUTRAL; 
    }
    //Serial.println(servoX);
    //Serial.print(", ");
    //Serial.println(translateY);
    if((servoX <= SERVO_1_MAX) && (servoX >= SERVO_1_MIN)) {
      move_servo(1, servoX);
    }
    //move_servo(0, translateY);

    //move_servo(1, constrain(servoX, SERVO_1_MIN, SERVO_1_MAX));
    //Serial.println(servoX);
  }
  delay(100);
  //tv.resume();
}

// misc tv functions
void initOverlay() {
  TCCR1A = 0;
  // Enable timer1.  ICES0 is set to 0 for falling edge detection on input capture pin.
  TCCR1B = _BV(CS10);

  // Enable input capture interrupt
  TIMSK1 |= _BV(ICIE1);

  // Enable external interrupt INT0 on pin 2 with falling edge.
  EIMSK = _BV(INT0);
  EICRA = _BV(ISC11);
}

void initInputProcessing() {
  // Analog Comparator setup
  ADCSRA &= ~_BV(ADEN); // disable ADC
  ADCSRB |= _BV(ACME); // enable ADC multiplexer
  ADMUX &= ~_BV(MUX0);  // select A2 for use as AIN1 (negative voltage of comparator)
  ADMUX |= _BV(MUX1);
  ADMUX &= ~_BV(MUX2);
  ACSR &= ~_BV(ACIE);  // disable analog comparator interrupts
  ACSR &= ~_BV(ACIC);  // disable analog comparator input capture
}

// Required
ISR(INT0_vect) {
  display.scanLine = 0;
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
    PROTOCOL_BYTE, DEVICE_NUM, SET_TARGET_BYTE, channel, pos & 0x7F, (pos >> 7) & 0x7F                 };
  for(int i = 0; i < 6; i++) {
    mySerial.write(command[i]);
    //Serial.println(command[i],HEX);
  } 
}

// move camera left right
void sweep() {
  int pos = SERVO_1_NEUTRAL;
  move_servo(1, pos);
  //sweep left
  for(pos; pos < SERVO_1_MAX; pos += 50) {
    move_servo(1, pos);
    delay(10); 
  }
  delay(200);
  //sweep right
  for(pos; pos > SERVO_1_MIN; pos -= 50) {
    move_servo(1, pos);
    delay(10); 
  }
  delay(200);
  // move back to center
  for(pos; pos < SERVO_1_NEUTRAL; pos += 50) {
    move_servo(1, pos);
    delay(10);
  }
}

// move camera head up/down
void pan() {
  int pos = SERVO_0_NEUTRAL;
  move_servo(0, pos);
  // look up
  for(pos; pos < SERVO_0_MAX; pos += 50) {
    move_servo(0, pos);
    delay(10);
  }
  delay(200);
  // look down
  for(pos; pos > SERVO_0_MIN; pos -= 50) {
    move_servo(0, pos);
    delay(10);
  }
  delay(200);
  // move to level
  for(pos; pos < SERVO_0_NEUTRAL; pos += 50) {
    move_servo(0, pos);
    delay(10); 
  }
}

void center_servos() {
  move_servo(4, SERVO_4_NEUTRAL);
  move_servo(3, SERVO_3_NEUTRAL);
  move_servo(2, SERVO_2_NEUTRAL);
  move_servo(1, SERVO_1_NEUTRAL);
  move_servo(0, SERVO_0_NEUTRAL); 
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
































