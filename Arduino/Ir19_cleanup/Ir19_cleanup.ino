
#define SAMPLES 128
#define MAXTIME 10000000
#define MAXTIME_DATAGRAM_MS 32

//-------------------------------------------------------------
// CONFIGURATION
//-------------------------------------------------------------

int led = 13;
int sensorPinInput = 9;
int sensorPinOutput = 6;

//-------------------------------------------------------------
// CAPTURED VALUES
//-------------------------------------------------------------
int power_ = 0;
int direction_ = 0;
int adjustment_ = 0;
int pitch_ = 0;
boolean channelA_ = false; 
boolean channelB_ = false; 

//-------------------------------------------------------------
// CODE
//-------------------------------------------------------------

unsigned long firstTime = 0;

unsigned long samples[SAMPLES];
int samplePos=0;

void setup() {
  Serial.begin(115200);
  while(!Serial);  
  Serial.println("Ready!");
  pinMode(led, OUTPUT);     
  pinMode(sensorPinInput, INPUT);       
  pinMode(sensorPinOutput, OUTPUT);         
  
  digitalWrite(sensorPinOutput, HIGH);    // turn off IR from the keyesIR
  reset();
}

unsigned long lastTime = 0;

// Ranges for minimum and maximum amplitudes for current encoding
#define MIN_SINGLE 300
#define MAX_SINGLE 600

#define MIN_DOUBLE 700
#define MAX_DOUBLE 1200

char text[32];

int getValue(unsigned long *buffer, int begin_, int numberBits = 8, boolean dump=true) {

  int value = 0;
  int bits = 0;

  boolean nextBit = false;
  for (int t=begin_; t<begin_+numberBits; t++) {   
    int len = buffer[t]-buffer[t-1]; 
    if (len>MIN_SINGLE && len<MAX_SINGLE) {
      if (dump) {
        Serial.print("0");
      } 
    } else if (len>MIN_DOUBLE && len<MAX_DOUBLE) {
        if (dump) {
          Serial.print("1");
        } 
        value|=(1<<bits);
      }   
    bits++;
  } 

  if (dump) 
    Serial.print(" ");
  return value;
}

void print_2(int value) { 
  sprintf(text, " %02d ", value); 
  Serial.print(text);
}

void print_3(int value) {
  sprintf(text, " %03d ", value); 
  Serial.print(text);
}

boolean fail = true;

// Full adjustment 
// B   POWER 157    A 00    A 00  RL  31        OK  \./ 10111001 ['\/'\./'\/\./]   00000100 [\/\/\./\/]         00000000 [\/\/\/\/]          01111100 [\./'\./'\./\/]     11011111 ['\./\./'\./'\./]    '

// Top left
// B   POWER 185    L 31    U 31  RL  31        OK  \./ 10011101 ['\/\./'\./\./]   11111101 ['\./'\./'\./\./]   11111001 ['\./'\./'\/\./]    01111100 [\./'\./'\./\/]     10101010 ['\/'\/'\/'

//  H. PPPPPPPP LLLLLABD UUUUU**D *AAAAA** CHECKSUM \./ 10011101 ['\/\./'\./\./]   00000100 [\/\/\./\/]         00000000 [\/\/\/\/]          00000000 [\/\/\/\/]          10011011 ['\/\./'\/'\./]   

void extract_values()
{
  boolean debug = false;  
  boolean debug_checksum = false;

  // CHANNEL A OR B
  int A1_ = getValue(samples,2+8+6, 1,false);
  int B1_ = getValue(samples,2+8+5, 1,false);

  if (debug) {
    if (A1_==1) Serial.print(" A ");
    if (B1_==1) Serial.print(" B ");
  }
  
  channelA_ = A1_;
  channelB_ = B1_;  

  int mask = (A1_<<6 | B1_<< 5);

  // POWER 
  int power = getValue(samples,2+0, 8, false);
  if (debug) {
    Serial.print(" POWER");
    print_3(power);
  }
  
  power_ = power;

  // LEFT RIGHT ANALOG CONTROL
  int left = getValue(samples,2+8+7, 1,false);
  int value_analog = getValue(samples,2+8, 5, false);

  if (debug) {
    if (value_analog!=0) {
      if (left) Serial.print(" L"); 
      else Serial.print(" R"); 
    } 
    else 
      Serial.print(" A");
  }
    
  if (left) 
    direction_ = -value_analog;
  else 
    direction_ = value_analog;
    
  if (debug)
    print_2(value_analog);

  // UP AND DOWN ANALOG CONTROL
  int up = getValue(samples,2+16+7, 1,false);
  value_analog = getValue(samples,2+16, 5, false);

  if (up) 
    pitch_ = -value_analog;
  else 
    pitch_ = value_analog;

  if (debug) {
    if (value_analog!=0) {
      if (up) Serial.print(" U"); 
      else Serial.print(" D"); 
    } 
    else 
      Serial.print(" A");    
    print_2(value_analog);
    Serial.print(" RL"); 
  }

  // ADJUSTMENTS DIGITAL
  value_analog=getValue(samples,2+24+1, 5, false);
  if (debug) 
    print_2(value_analog);
  
  adjustment_ = value_analog;

  // CHECKSUM = SUM ALL BYTES
  int checksum_value = getValue(samples,2+32, 8, false);

  int n0 = getValue(samples,2+0, 8, false);   
  int n1 = getValue(samples,2+8, 8, false);  
  int n2 = getValue(samples,2+16, 8, false);  
  int n3 = getValue(samples,2+24, 8, false);  

  unsigned char checksum = (n0+n1+n2+n3);  
  if (checksum != checksum_value) {
    Serial.print(" FAIL ");
    fail = true;
  } 
  else {
    if (debug)
      Serial.print("  OK  ");  
    fail = false;
  }
  
  if (debug)  
    Serial.println();
}

int lastTimeMs = 0;

void reset() {  
  digitalWrite(led, LOW);      
  samplePos = 0;  
  lastTimeMs = 0; 

  for (int t=0; t<SAMPLES; t++) {
    samples[SAMPLES] = 0;
  }  

  firstTime = micros();    
}

// the loop routine runs over and over again forever:
boolean capturing = false;
  
void loop() {
  
  if (!digitalRead(sensorPinInput)) {
    capturing = true;
  } else {
    YOUR_CODE_GOES_HERE();
  }
  
  while (capturing) {
    unsigned long samplingTime = micros()-firstTime;
    int elapsed = millis()-lastTimeMs;        
    if ((lastTimeMs!=0 && elapsed>MAXTIME_DATAGRAM_MS) && samplePos!=0) {      
      extract_values();
      reset();
      capturing = false;     
    } else {
    
      if (!digitalRead(sensorPinInput)) {                          
        if (lastTimeMs==0) {
          lastTimeMs = millis();
          firstTime = micros();       
        }
    
        samples[samplePos++]=micros()-firstTime;
        while (!digitalRead(sensorPinInput)) {        
        }       
        samples[samplePos++]=micros()-firstTime;      
        //Serial.print("S");
      }
    }
  }
}

//------------------ YOUR CODE GOES HERE ---------------------------------------

int count = 0;
void YOUR_CODE_GOES_HERE() {
    
  count++;
  if (count%1000==0) {

    if (channelA_) 
      Serial.print("A ");

    if (channelB_) 
      Serial.print("B ");
    
    Serial.print(power_, DEC);
    Serial.print(" ");
    Serial.print(pitch_, DEC);    
    Serial.print(" ");
    Serial.print(direction_, DEC);            
    Serial.print(" ");
    Serial.print(adjustment_, DEC);            
    Serial.println();    
  }
}
