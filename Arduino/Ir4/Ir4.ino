
#define SAMPLES 2048
#define MAXTIME 10000000
#define MAXTIME_MS 5000
#define MAXTIME_DATAGRAM 32000

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

void dumpData() {
    
  Serial.print("- DUMP DATA -");
  Serial.println(samplePos, DEC);  
  
  if (samplePos==0)
    return;
  
  playbackData();

  Serial.println("- SAMPLE DUMP -");
  
  for (int t=1; t<samplePos-1; t++) {   
    if (t%2) {
       Serial.print("ON  ");
    } else {
       Serial.print("OFF ");
    }
    Serial.print(t,DEC);
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

}

int lastTimeMs = 0;

void reset() {  
  digitalWrite(led, LOW);      
  samplePos = 0;  
  lastTimeMs = millis();  

  for (int t=0; t<SAMPLES; t++) {
       samples[SAMPLES] = 0;
  }  
  
  firstTime = micros();    
}

// the loop routine runs over and over again forever:

void loop() {
  
    loopValue++;   
    
    unsigned long samplingTime = micros()-firstTime;           
   
   /*
    Serial.print(samplingTime, DEC);              
    Serial.print(",");    
    Serial.print(millis(), DEC);                  
    Serial.println("ms");        
    */
    
    if (samplePos==0) {
        reset();
    }
    
    if (samplePos!=0 && (samplePos>SAMPLES-2 || samplingTime>MAXTIME)) {
        Serial.print("-- !!!SAMPLE TIME!!! --- ");      
        Serial.println(samplingTime, DEC);              
        dumpData();          
        reset();
  
    } else {
    
        int elapsed = millis()-lastTimeMs;
        if (lastTimeMs!=0 && elapsed>MAXTIME_MS) {
            dumpData();
        }
        
        if (!digitalRead(sensorPinInput)) {                    
          samples[samplePos++]=micros()-firstTime;
          while (!digitalRead(sensorPinInput)) {
            
          }       
          samples[samplePos++]=micros()-firstTime;
          
          lastTimeMs = millis();      
        }
    }
}

