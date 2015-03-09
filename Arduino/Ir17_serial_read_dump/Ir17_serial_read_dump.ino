
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

int sensorCharging = 7;

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
  pinMode(sensorCharging, INPUT_PULLUP);
}


// Full adjustment 
// B   POWER 157    A 00    A 00  RL  31        OK  \./ 10111001 ['\/'\./'\/\./]   00000100 [\/\/\./\/]         00000000 [\/\/\/\/]          01111100 [\./'\./'\./\/]     11011111 ['\./\./'\./'\./]    '

// Top left
// B   POWER 185    L 31    U 31  RL  31        OK  \./ 10011101 ['\/\./'\./\./]   11111101 ['\./'\./'\./\./]   11111001 ['\./'\./'\/\./]    01111100 [\./'\./'\./\/]     10101010 ['\/'\/'\/'

// Posible bits for more channels 
//  H. PPPPPPPP LLLLLABD UUUUU**D *AAAAA** CHECKSUM \./ 10011101 ['\/\./'\./\./]   00000100 [\/\/\./\/]         00000000 [\/\/\/\/]          00000000 [\/\/\/\/]          10011011 ['\/\./'\/'\./]   

void dumpBinary(unsigned char number) {
  for (int t=0; t<8; t++) {
    if (number&0x01) Serial.print(1,DEC); 
    else Serial.print(0,DEC);
    number>>=1;
  }
  Serial.print(" ");
}

boolean verbose = false;
boolean verboseDebug = false;

void sendCommand(unsigned int power, int leftright, int updown, int adjustments, boolean channelA, boolean ChannelB, boolean channelC = false, boolean ChannelD = false)
{
  unsigned char buffer[6];
  buffer[0]=power;

  unsigned char maskChannels = ~(1<<6 | 1<<5);

  int left=0;
  if (leftright<0) {
    left = 1<<7;
    leftright=-leftright;
  }

  buffer[1]=(leftright & maskChannels) | (channelA<<6 | ChannelB<<5) | left;

  int up=0;
  if (updown<0) {
    up = 1<<7;
    updown=-updown;
  }

  buffer[2]=(updown & maskChannels) | (channelC<<6 | ChannelD<<5) | up;
  buffer[3]=adjustments<<1;

  buffer[4]=0;  // Checksum
  for (int t=0; t<4; t++) {
    if (verbose)
      dumpBinary(buffer[t]);
    buffer[4]+=buffer[t];  
  } 

  playBuffer(buffer, 5);
}

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
  sendCommand(0,0,0,0,0,1);
  delay(1000);
  sendCommand(185,0,0,0,0,1);

  space(0); // Just to be sure
  digitalWrite(led, LOW);    
  delay(100);
}

int data_count = 0;
int data_stream = 0;
int sensor = 0;

#define BUFFER_SIZE 32
#define DATAGRAM_SIZE 7
unsigned char buffer[BUFFER_SIZE];

unsigned long lastSerialEvent = 0;

boolean error = false;
void reset()
{
  Serial.println("DATA RESET"); 
  data_count=0;
  data_stream=0;
}

void serialEvent() {

  unsigned int elapsed = millis()-lastSerialEvent;
  if (lastSerialEvent!=0 && elapsed>1000) {
    Serial.println("Timeout"); 
    reset();
  }

  boolean datagram = false;
  while (Serial.available()) {
    buffer[data_count%BUFFER_SIZE] = (char) Serial.read(); 

    if (error) {
      Serial.print(data_count, DEC); 
      Serial.print(","); 
      Serial.print(data_stream, DEC); 
      Serial.print(" "); 
      Serial.println(buffer[data_count%BUFFER_SIZE], DEC); 
      if (buffer[data_count%BUFFER_SIZE]!='B') {
        data_count=0;
        data_stream=0;
        return;
      } 
      error =false;
    }

    data_count++;
  }
}


char nextByte()
{
  char ret = buffer[data_stream%BUFFER_SIZE];
  data_stream++;
  return ret;
}

int old_power=0;
int old_leftright=0 ;
int old_updown=0;
int old_adjustments=0;
int old_channel=0;

int oldSensorVal=-1;

int lastBatteryEvent = false;
int chargingStart = 0;

boolean led_battery = true;
boolean dumpStatus = false;
void batteryCheck()
{
  // Charging check
  int sensorVal = digitalRead(sensorCharging);

  unsigned int elapsed = millis()-lastBatteryEvent;

  if (lastBatteryEvent==0) 
    lastBatteryEvent = millis();

  if (lastBatteryEvent!=0 && elapsed>2000) {
    if (!dumpStatus) {
      if (!sensorVal) {
        digitalWrite(led, HIGH);
        Serial.print(" Charging ");
      } 
      else {
        Serial.print(" Not charging ");
      }
      dumpStatus=true;
    }
  }

  if (sensorVal==oldSensorVal) 
    return;

  oldSensorVal = sensorVal;

  if (lastBatteryEvent!=0) {
    if (sensorVal && elapsed<100) {
      if (led_battery)
        digitalWrite(led, HIGH); 
      else
        digitalWrite(led, LOW); 
      led_battery=!led_battery;
    }

    /*
    if (sensorVal) {
     Serial.print(" ON  "); 
     } 
     else 
     Serial.print(" OFF ");
     
     Serial.println(elapsed,DEC);
     */
  }

  lastBatteryEvent = millis();
} 

void loop() {

  if (data_count>=(data_stream+DATAGRAM_SIZE)) {

    char command = nextByte();
    if (command=='V') {
      Serial.println("Verbose Activated"); 
      verboseDebug=true;
      reset();
      return;
    } 

    if (command!='B') {
      Serial.println("Missing Begin"); 
      reset();
      error=true;
      return;
    }

    int power = nextByte();
    int leftright = nextByte();
    int updown = nextByte();
    int adjustments = nextByte();
    int channel = nextByte();
    if (nextByte()!='E') {
      Serial.println("Missing End"); 
      reset();
      error=true;
      return;
    }

    if (verboseDebug) {
      if (old_power!=power || leftright!=old_leftright || updown!=old_updown || old_adjustments!=adjustments || channel!=old_channel) {
        old_power=power;
        old_leftright=leftright;
        old_updown=updown;
        old_adjustments=adjustments; 
        old_channel=channel;
        verbose=true;
      } 
      else {
        verbose=false;
      }
    }

    sendCommand(power, leftright, updown, adjustments, (channel&0x01)!=0, (channel&0x02)!=0, (channel&0x04)!=0, (channel&0x08)!=0);

    if (verbose) {
      if (channel&0x01)
        Serial.print(" A ");
      if (channel&0x02)
        Serial.print(" B "); 
      if (channel&0x04)
        Serial.print(" C "); 
      if (channel&0x08)
        Serial.print(" D "); 

      sprintf(text, " %03d %02d %02d %02d %d ", power,leftright,updown,adjustments,channel); 
      Serial.println(text);
    }
    // Discard data not processed. The serial event happens at the end of this loop.
    data_count=0;
    data_stream=0;
  }

  batteryCheck();
  //playRandom();
}




















