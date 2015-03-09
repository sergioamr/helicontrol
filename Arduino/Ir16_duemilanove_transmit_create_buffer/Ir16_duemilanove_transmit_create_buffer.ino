
#define SAMPLES 256
#define MAXTIME 10000000
#define MAXTIME_DATAGRAM_MS 32

#include <avr/interrupt.h>

#define CLKFUDGE 5      // fudge factor for clock interrupt overhead
#define CLK 256      // max value for clock (timer 2)
#define PRESCALE 8      // timer2 clock prescale
#define SYSCLOCK 16000000  // main Arduino clock
#define CLKSPERUSEC (SYSCLOCK/PRESCALE/1000000)   // timer clocks per microsecond

int sensorPin = A0;
char text[32];
  
int led = 13;
unsigned int samples[SAMPLES];
int samplePos=0;

void mark(int time) {
  // Sends an IR mark for the specified number of microseconds.
  // The mark output is modulated at the PWM frequency.
  TCCR2A |= _BV(COM2B1); // Enable pin 3 PWM output
  delayMicroseconds(time);
}

/* Leave pin off for time (given in microseconds) */
void space(int time) {
  // Sends an IR space for the specified number of microseconds.
  // A space is no output, so the PWM output is disabled.
  TCCR2A &= ~(_BV(COM2B1)); // Disable pin 3 PWM output
  delayMicroseconds(time);
}

void enableIROut(int khz) {
  // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
  // The IR output will be on pin 3 (OC2B).
  // This routine is designed for 36-40KHz; if you use it for other values, it's up to you
  // to make sure it gives reasonable results.  (Watch out for overflow / underflow / rounding.)
  // TIMER2 is used in phase-correct PWM mode, with OCR2A controlling the frequency and OCR2B
  // controlling the duty cycle.
  // There is no prescaling, so the output frequency is 16MHz / (2 * OCR2A)
  // To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
  // A few hours staring at the ATmega documentation and this will all make sense.
  // See my Secrets of Arduino PWM at http://arcfn.com/2009/07/secrets-of-arduino-pwm.html for details.


  // Disable the Timer2 Interrupt (which is used for receiving IR)
  TIMSK2 &= ~_BV(TOIE2); //Timer2 Overflow Interrupt

  pinMode(3, OUTPUT);
  digitalWrite(3, LOW); // When not sending PWM, we want it low

  // COM2A = 00: disconnect OC2A
  // COM2B = 00: disconnect OC2B; to send signal set to 10: OC2B non-inverted
  // WGM2 = 101: phase-correct PWM with OCRA as top
  // CS2 = 000: no prescaling
  TCCR2A = _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS20);

  // The top value for the timer.  The modulation frequency will be SYSCLOCK / 2 / OCR2A.
  OCR2A = SYSCLOCK / 2 / khz / 1000;
  OCR2B = OCR2A / 3; // 33% duty cycle
}

void setup() {
  Serial.begin(115200);
  while(!Serial);  
  Serial.println("Ready DUEMILANOVE!");
  pinMode(led, OUTPUT);     
  enableIROut(36);
}

void sendCommand(unsigned int power, int left, int right, int adjustments, boolean channelA, boolean ChannelB)
{
  unsigned char buffer[6];
  buffer[0]=power;
  buffer[1]=(channelA<<6 | ChannelB<< 5);
  buffer[2]=0;
  buffer[3]=0;

  buffer[4]=0;  // Checksum
  for (int t=0; t<4; t++) 
    buffer[4]+=buffer[t];   

  playBuffer(buffer, 5);
}

//   B   POWER 185    A 00    A 00  RL  00        OK  \./ 10011101 ['\/\./'\./\./]   00000100 [\/\/\./\/]         00000000 [\/\/\/\/]          00000000 [\/\/\/\/]          10011011 ['\/\./'\/'\./]   

void playBuffer(unsigned char *buffer, int length)
{
  unsigned int waittime;
  int count=0;

  mark(800); // HEADER
  for (int t=0; t<length; t++) { 

    for (int b=0; b<8;b++) {

      boolean bit_ = (buffer[t]>>b)&0x01;
      if (bit_%2)   
        waittime = 800;
      else 
        waittime = 400;

      if(++count%2)
        space(waittime);  
      else   
        mark(waittime);      

    }
  }
  space(0); // OFF
}

unsigned char power = 0;
void playRandom()
{
  digitalWrite(led, HIGH);      
  samplePos = SAMPLES;
  unsigned int counter = 0;

  //sendCommand(++power,0,0,0,1,0);

  sendCommand(185,0,0,0,0,1);

  space(0); // Just to be sure
  digitalWrite(led, LOW);    
  delay(100);
}

int data_count = 0;
int data_stream = 0;
int sensor = 0;

#define BUFFER_SIZE 128
unsigned char buffer[BUFFER_SIZE];

void serialEvent() {

  boolean datagram = false;
  while (Serial.available()) {
     buffer[data_count%BUFFER_SIZE] = (char) Serial.read(); 
     data_count++;
  }
}


void loop() {

  if (data_stream<(data_stream-6)) {
     int power = buffer[data_stream++];
     int left = buffer[data_stream++];
     int right = buffer[data_stream++];
     int adjustments = buffer[data_stream++];
     int channelA = buffer[data_stream++];
     int channelB = buffer[data_stream++];
     sendCommand(power, left, right, adjustments,  channelA, channelB);
     
     if (channelA)
         Serial.print(" A ");
     if (channelB)
         Serial.print(" B "); 
         
      sprintf(text, " %03d %02d %02d %02d ", power,left,right,adjustments); 
      Serial.println(text);
  }

  //playRandom();
}








