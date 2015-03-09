
#define SAMPLES 516
#define MAXTIME 10000000

int led = 13;
int sensorPinInput = 7;
int sensorPinOutput = 8;

int loopValue = 0;
int lastLoopValue = 0;

unsigned long firstTime = 0;

int samples[SAMPLES];
int samplePos=0;

void setup() {
  Serial.begin(115200);
  while(!Serial);  
  Serial.println("Ready!");
  pinMode(led, OUTPUT);     
  pinMode(sensorPinInput, INPUT);       
  pinMode(sensorPinOutput, OUTPUT);         
  
  digitalWrite(sensorPinOutput, LOW);    // turn off IR from the keyesIR
  firstTime = millis();  
  
  for (int t=0; t<SAMPLES; t++) {
       samples[SAMPLES] = 0;
  }
  
  Serial.println("- SETUP -");  
}

unsigned long lastTime = 0;

void printState(int state, int time) {
      Serial.print(time, DEC);              
      Serial.print(",");
      Serial.print(state, DEC);  
      Serial.println();            
}

void playbackData() {
  
  Serial.println("- PLAYBACK TIME WAIT -");
  
  digitalWrite(sensorPinOutput, LOW);
  delay(1000);
  Serial.println("- START -");  
  
  for (int t=1; t<samplePos-1; t++) {   
    if (t%2) {
      //Serial.print(" ON ");      
      digitalWrite(sensorPinOutput, HIGH);
    } else {
      //Serial.print(" OFF ");        
      digitalWrite(sensorPinOutput, LOW);
    }
    
    unsigned long waittime=samples[t]-samples[t-1];
    delay(waittime);
    Serial.println(waittime,DEC);  
  
  }    
  Serial.println("- END -");  
}

void dumpData() {

  Serial.print("- DUMP DATA -");
  Serial.println(samplePos, DEC);  
  
  if (samplePos==0)
    return;
  
  //playbackData();
  
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
  
  digitalWrite(led, LOW);      
  samplePos = 0;  
  firstTime = micros();              
}

// the loop routine runs over and over again forever:

void loop() {
  
    loopValue++;   
    
    unsigned long samplingTime = micros()-firstTime;           
   
    if (samplePos>SAMPLES || samplingTime>MAXTIME) {
        Serial.print("-- !!!SAMPLE TIME!!! --- ");      
        Serial.println(samplingTime, DEC);              
        dumpData();
    }
    
    if (!digitalRead(sensorPinInput)) {         
       int elapsed = micros()-lastTime;       
       if (lastTime==0 || elapsed>500000) {
          dumpData();          
      }      
      
      lastTime = millis();            
      samples[samplePos++]=micros()-firstTime;
      while (!digitalRead(sensorPinInput)) {
        
      }       
      samples[samplePos++]=micros()-firstTime;
    }
}

