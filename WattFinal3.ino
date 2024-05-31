#include <math.h>
#include <ZMPT101B.h>

const int currentSensorPin = A5; 
const int voltageSensorPin = A6; 

const float ADC_SCALE_VALUE = 1023.0;
const float VREF_VALUE = 5.0;
const float VOLTAGE_SENSOR_SCALE = 100.0; 
const float CURRENT_SENSOR_SCALE = 30.0; 

const int numSamples = 200;
int voltageSamples[numSamples];
int currentSamples[numSamples];

#define ACTectionRange 20
#define SENSITIVITY 302.5f
ZMPT101B voltageSensor(voltageSensorPin, 60.0);

float maxVoltage = 0;
float minVoltage = 5;

void setup() {
  Serial.begin(9600);
  voltageSensor.setSensitivity(SENSITIVITY);
}

void loop() {
  for (int i = 0; i < numSamples; i++) {
    voltageSamples[i] = analogRead(voltageSensorPin);
    currentSamples[i] = analogRead(currentSensorPin);
    delay(1); 
  }

  float voltageOffset = calculateMean(voltageSamples, numSamples) * (VREF_VALUE / ADC_SCALE_VALUE) * VOLTAGE_SENSOR_SCALE;
  float currentOffset = (calculateMean(currentSamples, numSamples) - 512) * (VREF_VALUE / ADC_SCALE_VALUE) * CURRENT_SENSOR_SCALE;

  float phaseDifference = calculatePhaseDifference(voltageSamples, currentSamples, numSamples, voltageOffset, currentOffset);
  float powerFactor = cos(phaseDifference);

  float ACCurrentValue = readACCurrentValue();
  float voltage = voltageSensor.getRmsVoltage();
  float power = fabs(ACCurrentValue * voltage * powerFactor);  // 절대값으로 변환

  Serial.print("Phase Difference: ");
  Serial.print(phaseDifference * 180 / PI); 
  Serial.println(" degrees");
  Serial.print("Power Factor: ");
  Serial.println(powerFactor);
  Serial.print("Current: ");
  Serial.print(ACCurrentValue);
  Serial.println(" A");
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.println(" V");
  Serial.print("Power: ");
  Serial.print(power);
  Serial.println(" W");
  Serial.println();

  delay(2000);
}

float calculateMean(int *samples, int numSamples) {
  long sum = 0;
  for (int i = 0; i < numSamples; i++) {
    sum += samples[i];
  }
  return (float)sum / numSamples;
}

float calculatePhaseDifference(int *voltageSamples, int *currentSamples, int numSamples, float voltageOffset, float currentOffset) {
  float sumProduct = 0.0;
  float sumVoltageSquared = 0.0;
  float sumCurrentSquared = 0.0;

  for (int i = 0; i < numSamples; i++) {
    float voltage = (voltageSamples[i] * (VREF_VALUE / ADC_SCALE_VALUE) * VOLTAGE_SENSOR_SCALE) - voltageOffset;
    float current = ((currentSamples[i] - 512) * (VREF_VALUE / ADC_SCALE_VALUE) * CURRENT_SENSOR_SCALE) - currentOffset;

    sumProduct += voltage * current;
    sumVoltageSquared += voltage * voltage;
    sumCurrentSquared += current * current;
  }

  return acos(sumProduct / sqrt(sumVoltageSquared * sumCurrentSquared));
}

float readACCurrentValue() {
  maxVoltage = 0;
  minVoltage = 5;

  for (int i = 0; i < 1000; i++) {
    float sensorValue = analogRead(currentSensorPin);  
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
  float currentAC = (peakToPeakVoltage / 2.0) / (2.506138 / 2.5) * 1000;

  if (currentAC <= 0) {
    currentAC = 0;
  }

  return currentAC / 1000.0; // Convert to Amperes
}
