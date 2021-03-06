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
#define MIN_RANGE 30.0
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
#define SERVO_LIGHT_SENSITIVITY 10
#define POLOLU_BIT_MASK 0x7F
#define SENSOR_READ_INTERVAL 10
#define FRAME_DELAY 0
#define DEFAULT_BACK_STEPS 1

const int gp2d12Pin = 5; // analog pin 5

TVout tv;

SoftwareSerial mySerial(pRxPin, pTxPin);

unsigned char c;
unsigned char minX,minY,maxX,maxY;
char s[32];

int servoX = SERVO_1_NEUTRAL;
int servoY = SERVO_0_NEUTRAL;

int read_sensor_counter = 0;
float current_range = 0.0;

void setup() {
  tv.begin(NTSC, W, H);
  initOverlay();
  initVideoProcessing();
  //Serial.begin(DEFAULT_BAUD);
  mySerial.begin(DEFAULT_BAUD);
  delay(1000);
  center_servos();
  //delay(1000);
  //walk_forward(2);
  //delay(1000);
  //walk_backward(2);
  tv.select_font(font4x6);
  tv.fill(0);
}

void loop() {
  if(read_sensor_counter > SENSOR_READ_INTERVAL) {
    read_sensor_counter = 0;
    current_range = read_gp2d12_range_adc();
    //Serial.println(range);
    if(current_range <= MIN_RANGE) {
      //center_servos();
      //servoY = SERVO_0_NEUTRAL;
      //servoX = SERVO_1_NEUTRAL;
      //delay(100);
      walk_backward(DEFAULT_BACK_STEPS);
    }
  }
  detect_light();
  delay(50);
  read_sensor_counter++;
}

void detect_light() {
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
    int rawX = (maxX+minX)/2;
    int translateX = map(rawX, 0, W, SERVO_1_MAX, SERVO_1_MIN);

    int rawY = (maxY+minY)/2;
    int translateY = map(rawY, 0, H, SERVO_0_MAX, SERVO_0_MIN);
    // X axis calculations
    if(translateX > SERVO_1_NEUTRAL) {
      servoX = min(SERVO_1_MAX,(servoX + ((translateX - SERVO_1_NEUTRAL)/SERVO_LIGHT_SENSITIVITY)));
    } 
    else if (translateX < SERVO_1_NEUTRAL) {
      servoX = max(SERVO_1_MIN, (servoX - ((SERVO_1_NEUTRAL - translateX)/SERVO_LIGHT_SENSITIVITY))); 
    } 
    else {
      servoX = SERVO_1_NEUTRAL; 
    }
    if((servoX <= SERVO_1_MAX) && (servoX >= SERVO_1_MIN)) {
      move_servo(1, servoX);
    }
    if(servoX < SERVO_1_NEUTRAL) {
      turn_right(1); 
    } 
    else if (servoX > SERVO_1_NEUTRAL) {
      turn_left(1); 
    } else {
      walk_forward(1);
    }
    // Y axis calculations
    if(translateY > SERVO_0_NEUTRAL) {
      servoY = min(SERVO_0_MAX,(servoY + ((translateY - SERVO_0_NEUTRAL)/SERVO_LIGHT_SENSITIVITY)));
    } 
    else if (translateY < SERVO_0_NEUTRAL) {
      servoY = max(SERVO_0_MIN, (servoY - ((SERVO_0_NEUTRAL - translateY)/SERVO_LIGHT_SENSITIVITY))); 
    } 
    else {
      servoY = SERVO_0_NEUTRAL; 
    }
    if((servoY <= SERVO_0_MAX) && (servoY >= SERVO_0_MIN)) {
      move_servo(0, servoY);
    }
  }
  /*tv.fill(0);
   if (found) {
   tv.draw_line(minX, minY, maxX, minY, 1);
   tv.draw_line(minX, minY, minX, maxY, 1);
   tv.draw_line(maxX, minY, maxX, maxY, 1);
   tv.draw_line(minX, maxY, maxX, maxY, 1);
   sprintf(s, "%d, %d", ((maxX+minX)/2), ((maxY+minY)/2));
   tv.print(0, 0, s);
   }
   tv.resume();
   tv.delay_frame(FRAME_DELAY);*/
}

// can we read the distance sensor using the ADC
float read_gp2d12_range_adc() {
  ADCSRB &= ~_BV(ACME); // disable ADC multiplexer
  ADCSRA |= _BV(ADEN); // enable ADC
  delay(2);
  float val = read_gp2d12_range(gp2d12Pin);
  if(val < 0) {
    val = MIN_RANGE; 
  }
  //delay(2);
  //initOverlay();
  initVideoProcessing();
  return val;
}

// misc tv functions
void initOverlay() {
  TCCR1A = 0; // disable PWM on timer 1 during setup
  // Enable timer1.  ICES0 is set to 0 for falling edge detection on input capture pin.
  TCCR1B = _BV(CS10);

  // Enable input capture interrupt
  TIMSK1 |= _BV(ICIE1);

  // Enable external interrupt INT0 on pin 2 with falling edge.
  EIMSK = _BV(INT0);
  EICRA = _BV(ISC11);
}

void initVideoProcessing() {
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
  analogRead(pin);
  delay(10);
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
    PROTOCOL_BYTE, DEVICE_NUM, SET_TARGET_BYTE, channel, pos & POLOLU_BIT_MASK, (pos >> 7) & POLOLU_BIT_MASK};
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













































