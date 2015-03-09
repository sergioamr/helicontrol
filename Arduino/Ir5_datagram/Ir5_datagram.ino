
#define SAMPLES 1024
#define MAXTIME 10000000
#define MAXTIME_DATAGRAM_MS 32

int led = 13;
int sensorPinInput = 7;
int sensorPinOutput = 6;

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

// Ranges for minimum and maximum amplitudes for manchester encoding
#define MIN_SINGLE 400
#define MAX_SINGLE 600

#define MIN_DOUBLE 700
#define MAX_DOUBLE 1200

unsigned char lastBuffer[128];

unsigned char samplesProcess[128];

int samenum= 0;
void dumpDecodeManchester() {
  
  boolean bit_ = 1;
  
  boolean same = true;

  int pos = 0;
  int value = 1;
  
  for (int t=1; t<samplePos; t++) {   
    
    if (t%2) {
       bit_ = 1;
    } else {
       bit_ = 0;
    }
    
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
  
  if (same) {
      samenum++;
      if (samenum>2)
          Serial.print("*");  
      return;
  }
  
  if (samenum>0)
      Serial.println(" ");  
      
  samenum=0;
 /*
  for (int t=0; t<pos; t++) {   
     Serial.print(samplesProcess[t]);  
  } 
  
  Serial.print("    ");  
  Serial.println(pos, DEC);   
  */
  Serial.print("'"); 
  
  for (int t=1; t<pos; t++) {   
     if (samplesProcess[t]==0 && samplesProcess[t-1]==1) {
          Serial.print("\\");  
          value = 0;
     } else 
     if (samplesProcess[t]==1 && samplesProcess[t-1]==0) {
          Serial.print("/"); 
          value = 1;
     } else {
          if (value==1) 
              Serial.print("'"); 
          else
              Serial.print("."); 
     }    
  } 
  
  Serial.print(" ");
    Serial.println(pos, DEC);   

/*
  value = 1;
  for (int t=1; t<pos; t++) {   
     if (samplesProcess[t]==0 && samplesProcess[t-1]==1) {
          value = 0;
     } else 
     if (samplesProcess[t]==1 && samplesProcess[t-1]==0) {
          value = 1;
     } 
     
     if (t%2==0) {
         Serial.print(value, DEC);   
         Serial.print(" "); 
     }
  } 

  Serial.println(" ");
  */
}

void dumpData() {
    
  //Serial.print("- DUMP DATA -");
  //Serial.println(samplePos, DEC);  
  
  if (samplePos==0)
    return;
  
  for (int t=0; t<100; t++) 
  playbackData();
  
/*
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
 
*/
  dumpDecodeManchester();
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

/*
void dumpTime()
{
    Serial.print(samplingTime, DEC);              
    Serial.print(",");    
    Serial.print(millis(), DEC);                  
    Serial.println("ms");        
}
*/

// the loop routine runs over and over again forever:

void loop() {
  
    loopValue++;   
    
    unsigned long samplingTime = micros()-firstTime;           
           
    int elapsed = millis()-lastTimeMs;        
    if ((lastTimeMs!=0 && elapsed>MAXTIME_DATAGRAM_MS) && samplePos!=0) {      
        //Serial.println("------------DATAGRAM-----------");
        dumpData();
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

