#ifndef Timer
#define Timer

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1ms
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void TimerOn() {
	// AVR timer/counter controller register TCCR1
  TCCR1B 	= 0x0B;	// bit3 = 1: CTC mode (clear timer on compare)
					// bit2bit1bit0=011: prescaler /64
					// 00001011: 0x0B
					// SO, 16 MHz clock or 16,000,000 /64 = 250,000 ticks/s
					// Thus, TCNT1 register will count at 250,000 ticks/s
          // FOR MICROSECONDS:
          // bit3 = 1: CTC mode (clear timer on compare)
					// bit2bit1bit0=010: prescaler /64
					// 00001011: 0x0A
					// SO, 16 MHz clock or 16,000,000 /8 = 2,000,000 ticks/s
					// Thus, TCNT1 register will count at 2,000,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A 	= 250;	// Timer interrupt will be generated when TCNT1==OCR1A
					// We want a 1 ms tick. 0.001 s * 250,000 ticks/s = 250
					// So when TCNT1 register equals 250,
					// 1 ms has passed. Thus, we compare to 250.
					// AVR timer interrupt mask register
          // FOR MICROSECONDS:
          // Timer interrupt will be generated when TCNT1==OCR1A
					// We want a 1 us tick. 0.000001 s * 2,000,000 ticks/s = 2
					// So when TCNT1 register equals 2,
					// 1 us has passed. Thus, we compare to 2.
					// AVR timer interrupt mask register

	TIMSK1 	= 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1 = 0;

	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;

	//Enable global interrupts
	SREG |= 0x80;	// 0x80: 1000000
}

void TimerOff() {
	TCCR1B 	= 0x00; // bit3bit2bit1bit0=0000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect)
{
	// CPU automatically calls when TCNT0 == OCR0 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; 			// Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { 	// results in a more efficient compare
		TimerISR(); 				// Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

#endif //Timer

#include <IRremote.h>
#define IR_RECEIVE_PIN 9

int button = 0; //stores the value for specific remote button
int cnt  = 1; //stores the number on 4 digit 7 segment display

enum P_STATES {WAIT_PRESS, HIGH_PRESS, LOW_PRESS} P_State = WAIT_PRESS;
void tick_pitch(void) {
  // transitions
  switch (P_State) {
    case WAIT_PRESS:
      if(button == 2) {
        if(cnt <= 20) {
          cnt++; //increment count unless at the upper limit of 20
        }
        P_State = HIGH_PRESS;
      }
      else if(button == 3) {
        if(cnt > 0) {
          cnt--; //decrement count unless at 0
        }
        P_State = LOW_PRESS;
      }
      else {
        P_State = WAIT_PRESS;
      }
      break;
    case HIGH_PRESS:
      if(button == 2) {
        P_State = HIGH_PRESS;
      }
      else {
        P_State = WAIT_PRESS;
      }
      break;
    case LOW_PRESS:
      if(button == 3) {
        P_State = LOW_PRESS;
      }
      else {
        P_State = WAIT_PRESS;
      }
      break;
  }
  // actions
  switch (P_State) {
    case WAIT_PRESS:
      break;
    case HIGH_PRESS:
      break;
    case LOW_PRESS:
      break;
  }
}

int i = 0; //used to control buzzer length time
enum B_STATES {WAIT_BUZZ, BUZZ_ON, BUZZ_OFF, IDLE} B_State = WAIT_BUZZ;
void tick_buzz(void) {
  // transitions
  switch (B_State) {
    case WAIT_BUZZ:
      if(button == 31) {
        digitalWrite(11, HIGH);
        B_State = BUZZ_ON;
      }
      else {
        B_State = WAIT_BUZZ;
      }
      break;
    case BUZZ_ON:
      if(i > (55-(cnt*5))) {
        digitalWrite(11, LOW);
        i = 0;
        B_State = BUZZ_OFF;
      }
      else {
        ++i;
        B_State = BUZZ_ON;
      }
      break;
    case BUZZ_OFF:
      if(button == 31) {
        i = 0;
        B_State = IDLE;
      }
      else if(i > (55-(cnt*5))) {
        digitalWrite(11, HIGH);
        i = 0;
        B_State = BUZZ_ON;
      }
      else {
        ++i;
        B_State = BUZZ_OFF;
      }
      break;
    case IDLE: //used to wait for pausing
      if(i > 50) {
        i = 0;
        B_State = WAIT_BUZZ;
      }
      else {
        ++i;
        B_State = IDLE;
      }
      break;
  }
  // actions
  switch (B_State) {
    case WAIT_BUZZ:
      break;
    case BUZZ_ON:
      break;
    case BUZZ_OFF:
      break;
    case IDLE:
      break;
  }
}

char gSegPins[] = {2,3,4,5,6,7,8};
//function used from lab 6
void displayNumTo7Seg(unsigned int targetNum, int digitPin) {
    unsigned char encodeInt[10] = {
        // 0     1     2     3     4     5     6     7     8     9
        0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67,
    };
    // Make sure the target digit is off while updating the segments iteratively
    digitalWrite(digitPin, HIGH);
    // Update the segments
    for (int k = 0; k < 7; ++k) {
        digitalWrite(gSegPins[k], encodeInt[targetNum] & (1 << k));
    }
    // Turn on the digit again
    digitalWrite(digitPin, LOW);
}

void setup() {
  TimerSet(10);
  TimerOn();
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(11, OUTPUT);
  digitalWrite(A1, HIGH);
  digitalWrite(A2, HIGH);
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN);
}
void loop() {
  if (IrReceiver.decode()) {
    IrReceiver.resume();
    //Serial.println(IrReceiver.decodedIRData.command);
    button = IrReceiver.decodedIRData.command;
    if(button == 2) {
      Serial.println("higher pitch");
    }
    else if(button == 3) {
      Serial.println("lower pitch");
    }
    else if(button == 31) {
      Serial.println("play/stop");
      //digitalWrite(11, HIGH);
    }
  }
  tick_pitch(); //buzzer speed display SM
  tick_buzz(); //SM for buzz siren sound with peroids depending on cnt
  if(cnt < 10) {
    displayNumTo7Seg(cnt, A1);
    digitalWrite(A2, HIGH);
    while(!TimerFlag){}
    TimerFlag = 0;
    displayNumTo7Seg(0, A2);
    digitalWrite(A1, HIGH);
    while(!TimerFlag){}
    TimerFlag = 0;
  }
  else {
    displayNumTo7Seg((cnt%10), A1);
    digitalWrite(A2, HIGH);
    while(!TimerFlag){}
    TimerFlag = 0;
    displayNumTo7Seg((cnt/10), A2);
    digitalWrite(A1, HIGH);
    while(!TimerFlag){}
    TimerFlag = 0;
  }
}