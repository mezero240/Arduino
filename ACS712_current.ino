const int analogInPin = A5;  
float voltage;               
float currentAC;             
float maxVoltage = 0;
float minVoltage = 5;

void setup() {
  Serial.begin(9600);  
}

void loop() {
  maxVoltage = 0;
  minVoltage = 5;

  for (int i = 0; i < 1000; i++) {
    float sensorValue = analogRead(analogInPin);  
    float voltage = (sensorValue / 1023.0) * 5.0; 
    if (voltage > maxVoltage) {
      maxVoltage = voltage;
    }
    if (voltage < minVoltage) {
      minVoltage = voltage;
    }
    delayMicroseconds(100); 
  }

   float peakToPeakVoltage = maxVoltage - minVoltage;
  float zeroPoint = (maxVoltage + minVoltage) / 2.0; 
  currentAC = (peakToPeakVoltage / 2.0) / (2.506138 / 2.5) * 1000;  

  if (currentAC <= 0) {
    currentAC = 0;
  }

  Serial.print(currentAC);
  Serial.println("mA");
  delay(1000);
}