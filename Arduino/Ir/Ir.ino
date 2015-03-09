
int led = 13;
int sensorPinInput = 7;
int sensorPinOutput = 8;

int loopValue = 0;
int lastLoopValue = 0;

void setup() {
  Serial.begin(9600);
  while(!Serial);  
  Serial.println("Ready!");
  pinMode(led, OUTPUT);     
  pinMode(sensorPinInput, INPUT);       
  pinMode(sensorPinOutput, OUTPUT);         
  
  digitalWrite(sensorPinOutput, LOW);    // turn off IR from the keyesIR
}

int lastTime = 0;

// the loop routine runs over and over again forever:
void loop() {

    loopValue++;
    
    if (!digitalRead(sensorPinInput)) {   
      //Serial.print("DOWN ");
      //Serial.println(millis()-lastTime, DEC);              

      if (lastTime==0) {
          lastTime = millis();
      }
      Serial.println(millis(), DEC);              

      lastTime = millis();
      
      digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)

      //Serial.print("  UP ");
      while (!digitalRead(sensorPinInput)) {
      }     
      //Serial.println(millis()-lastTime, DEC);              
      lastTime = millis();
    
      digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW  
    }

}

