#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#ifndef cbi
  #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
  #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

const int pResistor = PB3;
const int ledPin = PB1;

const int bright = 128;         // how bring you want the LED to get
const int ledOnTime = 20000;    // how long you want the LED to be on when it triggers
const int sleepX = 11;          // Sleep multiplier.  Max sleep is ~8 seconds, so if you want to sleep for ~90 seconds, 8x11 is close
const int lightThreshold = 250; // threshold to trigger LED

volatile boolean f_wdt = 1;   // sleep state
volatile int cnt = sleepX;         // keep track of sleeps.  Start over thrshold. 

void RampUpLED(int brightness);
void RampDownLED(int brightness);
void system_sleep();
void setup_watchdog(int ii);
ISR(WDT_vect);

void setup() 
{
  pinMode(pResistor, INPUT);
  pinMode(ledPin, OUTPUT);

  // To save power, put the ATTiny
  setup_watchdog(9);
}

void loop() 
{
  if (f_wdt == 1)
  {
    f_wdt = 0;

    if (cnt < sleepX)
    {
      cnt++;
      // not read, go back to sleep
      return;
    }
    else
    {
      cnt = 0;
      int lightValue = analogRead(pResistor);
      if (lightValue < lightThreshold)
      {
        pinMode(ledPin, OUTPUT);
        RampUpLED(bright);
        delay(ledOnTime);
        RampDownLED(bright);
        pinMode(ledPin, INPUT);   // supposed to save power.  My volt meter is from HF so I can't actually check this
      }
    }
  }  
}

// ramps up to ~brightness over 1 second (50ms X 20 segments).  Not exact, but doesn't need to be.
void RampUpLED(int brightness)
{
  int increment = brightness % 20;
  for (int i = 0; i < brightness; i = i + increment)
  {
    analogWrite(ledPin, i);
    delay(50);
  } 
}

// ramps down from brightness over 1 second (50ms X 20 segments).  Not exact, but doesn't need to be.
void RampDownLED(int brightness)
{
  int increment = brightness % 20;
  for (int i = brightness; i > 0; i = i - increment)
  {
    analogWrite(ledPin, i);
    delay(50);
  }
  // make sure it's off;
  digitalWrite(ledPin, LOW); 
}

// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep() {
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();

  sleep_mode();                        // System sleeps here

  sleep_disable();                     // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {

  byte bb;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}
  
// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
  f_wdt=1;  // set global flag
}