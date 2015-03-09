
#define SAMPLES 516
#define MAXTIME 10000

int led = 13;
int sensorPinInput = 7;
int sensorPinOutput = 8;

int loopValue = 0;
int lastLoopValue = 0;

int firstTime = 0;

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
  
}

int lastTime = 0;
int printStart = 0;

void printState(int state, int time) {
      Serial.print(time, DEC);              
      Serial.print(",");
      Serial.print(state, DEC);  
      Serial.println();            
}

void dumpData() {
  
  if (samplePos==0)
    return;
  
  Serial.println("- SAMPLE TIME -");

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
  firstTime = millis();              
}

// the loop routine runs over and over again forever:

void loop() {
  
    loopValue++;   
    
    int samplingTime = millis()-firstTime;           
   
    if (samplePos>SAMPLES || samplingTime>MAXTIME) {
        dumpData();
    }
    
    if (!digitalRead(sensorPinInput)) {         
       int elapsed = millis()-lastTime;       
       if (lastTime==0 || elapsed>500) {
          dumpData();          
      }      
      
      lastTime = millis();            
      samples[samplePos++]=millis()-firstTime;
      while (!digitalRead(sensorPinInput)) {
        
      }       
      samples[samplePos++]=millis()-firstTime;
    }
}

