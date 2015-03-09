
#define SAMPLES 1024
#define MAXTIME 10000000
#define MAXTIME_DATAGRAM_MS 32

int led = 13;
int sensorPinInput = 7;
int sensorPinOutput = 8;

int loopValue = 0;
int lastLoopValue = 0;

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
    
  digitalWrite(sensorPinOutput, LOW);    // turn on IR from the keyesIR
  
  reset();
}

unsigned long lastTime = 0;

void printState(int state, unsigned long time) {
      Serial.print(time, DEC);              
      Serial.print(",");
      Serial.print(state, DEC);  
      Serial.println();            
}

/*
void playbackData() {

  Serial.println("- PLAYBACK TIME WAIT -");
  
  // http://www.bristolwatch.com/arduino/arduino_ir.htm
  // The enable pin "EN" will disable the device when HI (Vcc) and enable when LO (GND). The onboard jumper can be left open to allow external control of enable/disable of the module. I see no use for this function and would leave the jumper on and the pin disconnected.
  
  digitalWrite(sensorPinOutput, HIGH); // 
  delay(1000);
  Serial.println("-- START --");  
  
  for (int t=1; t<samplePos-1; t++) {   
    
    unsigned long micro = micros();
    if (t%2) {
      //Serial.print(" ON ");      
      digitalWrite(sensorPinOutput, HIGH);
      //digitalWrite(led, HIGH);      
    } else {
      //Serial.print(" OFF ");      
      //digitalWrite(led, LOW);            
      digitalWrite(sensorPinOutput, LOW);
    }

    unsigned long waittime=samples[t]-samples[t-1];
    unsigned long elapsed= micros() - micro;        
    
    if (elapsed<waittime) {
       //Serial.println(waittime-elapsed,DEC);    
       delayMicroseconds(waittime-elapsed);
    }
      
  }    
  
  digitalWrite(sensorPinOutput, HIGH);
  Serial.println("-- END --");  
}
*/

// Ranges for minimum and maximum amplitudes for manchester encoding
#define MIN_SINGLE 400
#define MAX_SINGLE 600

#define MIN_DOUBLE 700
#define MAX_DOUBLE 1200

unsigned char lastBuffer[128];
unsigned char samplesProcess[128];
char text[32];
  
int getValue(unsigned long *buffer, int begin_, int numberBits = 8, boolean dump=true) {
    
  int value = 0;
  int bits = 0;
  
  boolean nextBit = false;
  for (int t=begin_; t<begin_+numberBits; t++) {   
    int len = buffer[t]-buffer[t-1]; 
    if (len>MIN_SINGLE && len<MAX_SINGLE) {
      if (dump)
         Serial.print(0,DEC); 
    } else 
    if (len>MIN_DOUBLE && len<MAX_DOUBLE) {
      if (dump)
         Serial.print(1,DEC); 
      value|=(1<<bits);
    }   
    bits++;
  } 
  
  if (dump) {
     sprintf(text, " %03d ", value); Serial.print(text);    
  }
  return value;
}
    
void print_2(int value) {
  sprintf(text, " %02d ", value); Serial.print(text);
}

void print_3(int value) {
  sprintf(text, " %03d ", value); Serial.print(text);
}

void extract_values()
{
  // CHANNEL A OR B
  int A1_ = getValue(samples,2+8+6, 1,false);
  int B1_ = getValue(samples,2+8+5, 1,false);
   
  if (A1_==1) Serial.print(" A ");
  if (B1_==1) Serial.print(" B ");
  
  int mask = (A1_<<6 | B1_<< 5);

   // POWER 1
  int power = getValue(samples,2+0, 8, false);
  Serial.print(" POWER");
  print_3(power);
  
  // POWER 2
  int value_analog = getValue(samples,2+32, 8, false);
  Serial.println(" [C] ");  
  Serial.print(" M ");
  Serial.println(mask|1<<9, BIN);
  
  Serial.print(" A ");
  Serial.println(1<<6 |1<<9, BIN);
  Serial.print(" B ");
  Serial.println(1<<5 |1<<9, BIN);  
  Serial.println(" ");
     
  Serial.print(" P ");
  Serial.println(power|1<<9, BIN);  
  Serial.print(" V ");
  Serial.println(value_analog|1<<9, BIN);
  
  Serial.println(" = ");
  Serial.print(" C ");
  int checksum = (power+mask);
  Serial.println(checksum|1<<9, BIN);  
  
  if (checksum == value_analog) 
      Serial.println(" CHECKSUM MATCH ");
  
  Serial.println(" ");
  Serial.print(" D ");
  int test = value_analog ^ power;
  Serial.println(test|1<<9, BIN);  
  
  Serial.println(" ");  
  
  print_3(checksum);
  
  // LEFT RIGHT ANALOG CONTROL
  int left = getValue(samples,2+8+7, 1,false);
  value_analog = getValue(samples,2+8, 5, false);
  
  if (value_analog!=0) {
      if (left) Serial.print(" L"); else Serial.print(" R"); 
  } else 
      Serial.print(" *");
  print_2(value_analog);
  
  // UP AND DOWN ANALOG CONTROL
  int up = getValue(samples,2+16+7, 1,false);
  value_analog = getValue(samples,2+16, 5, false);
  
  if (value_analog!=0) {
      if (up) Serial.print(" U"); else Serial.print(" D"); 
  } else 
      Serial.print(" *");
  print_2(value_analog);
    
  // ADJUSTMENTS DIGITAL
  Serial.print(" RL"); 
  value_analog=getValue(samples,2+24+1, 5, false);
  print_2(value_analog);
  
  Serial.println(""); 
}

int samenum= 0;

void dumpDecode() {
  
  boolean bit_ = 1;
  
  boolean same = true;

  int pos = 0;
  int value = 1;
  
  for (int t=0; t<samplePos; t++) {   
    
    if (t%2) bit_ = 1; else bit_ = 0;

    if (lastBuffer[pos]!=bit_)
      same=false;
    
    samplesProcess[pos]=bit_;
    lastBuffer[pos]=bit_;
    pos++;
    
    int len = samples[t]-samples[t-1];   
    if (len>MIN_SINGLE && len<MAX_SINGLE) {
    } else 
    if (len>MIN_DOUBLE && len<MAX_DOUBLE) {
        if (lastBuffer[pos]!=bit_)
           same=false;
           
        samplesProcess[pos]=bit_;
        lastBuffer[pos]=bit_;
        pos++;
    }
    
  }    
  
  samplesProcess[pos++]=0;
  
  if (same) {
      samenum++;
      if (samenum>2 && samenum<20)
          Serial.print("*");  
      return;
  }
  
  if (samenum>0)
      Serial.println(" ");  
      
  samenum=0;
  
  extract_values();
  return;
  
  int bits = 0; 
 
  value = 0;
  bits = 0;

  int segmentBegin = 0;
  int segmentBegin2 = 0;
  int segmentBegin3 = 0;
  int segmentBegin4 = 0;
  int segmentBegin5 = 0;
  
  int segmentEnd = 0;
  
  boolean header = false;
  
  int len =0; 
  for (int t=0; t<pos; t++) {   
    
     if (samplesProcess[t]==0 && samplesProcess[t+1]==1) {
          Serial.print("\\");  
          value = 0;
          bits++;
     } else 
     if (samplesProcess[t]==1 && samplesProcess[t+1]==0) {
          Serial.print("/"); 
          value = 1;
          bits++;
     } else {
          if (value==1) Serial.print("'"); else Serial.print("."); 
     }    
     
     if (bits==2 && !header)  {
         Serial.print(" "); 
         getValue(samples,2);
         Serial.print("["); 
         bits=0;
         header=true;
         segmentEnd=t;
     } 
 
     if (bits==8 && segmentBegin == 0)  {
         segmentBegin=t;
         Serial.print("]"); 
         for (int i=0; i<18-t; i++) Serial.print(" ");
         getValue(samples,2+8);  
         Serial.print("[");  
     }   
 
    if (bits==16 && segmentBegin2 == 0)  {
         segmentBegin2=t;
         Serial.print("]"); for (int i=0; i<18-(segmentBegin2-segmentBegin); i++) Serial.print(" "); 
         getValue(samples,2+16);   
         Serial.print("[");  
     }      
     
     if (bits==24 && segmentBegin3 == 0)  {
         segmentBegin3=t; 
         Serial.print("]"); for (int i=0; i<18-(segmentBegin3-segmentBegin2); i++) Serial.print(" ");   
         getValue(samples,2+24); 
         Serial.print("[");
     }   
     
     if (bits==32 && segmentBegin4 == 0)  {
         segmentBegin4=t; 
         Serial.print("]"); for (int i=0; i<18-(segmentBegin4-segmentBegin3); i++) Serial.print(" ");     
         getValue(samples,2+32); 
         Serial.print("["); 
     }  

     if (bits==40 && segmentBegin5 == 0)  {
         segmentBegin5=t; 
         Serial.print("] "); 
         for (int i=0; i<18-(segmentBegin5-segmentBegin4); i++) Serial.print(" ");
     }      
  } 
 
  Serial.println("");  
}

void dumpData() {
    
  //Serial.print("- DUMP DATA -");
  //Serial.println(samplePos, DEC);  
  
  if (samplePos==0)
    return;
  
  //playbackData();

  Serial.println("- SAMPLE DUMP -");
  
  for (int t=1; t<samplePos; t++) {   
    if (t%2) {
       Serial.print("ON  ");
    } else {
       Serial.print("OFF ");
    }
    Serial.print(t,DEC);
    Serial.print(",");  
    Serial.print(samples[t], DEC);   
    Serial.print(",");
    Serial.println(samples[t]-samples[t-1], DEC); 
  }

  Serial.println("-- SAMPLES ---");
  
  digitalWrite(led, HIGH);   
  for (int t=0; t<samplePos; t+=2) {
        printState(0,samples[t]);
        printState(1,samples[t]);      
        
        printState(1,samples[t+1]);
        printState(0,samples[t+1]);          
  }

  dumpDecode();
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

void loop() {
  
    loopValue++;   
    
    unsigned long samplingTime = micros()-firstTime;           
           
    int elapsed = millis()-lastTimeMs;        
    if ((lastTimeMs!=0 && elapsed>MAXTIME_DATAGRAM_MS) && samplePos!=0) {      
        //Serial.println("------------DATAGRAM-----------");
        dumpDecode();
        reset();
    }
                  
    if (!digitalRead(sensorPinInput)) {                          
      if (lastTimeMs==0) {
          lastTimeMs = millis();
          firstTime = micros();       
      }
          
      samples[samplePos++]=micros()-firstTime;
      while (!digitalRead(sensorPinInput)) {        
      }       
      samples[samplePos++]=micros()-firstTime;      
    }

}

