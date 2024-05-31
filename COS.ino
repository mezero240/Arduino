#include <math.h>

const int currentSensorPin = A5; 
const int voltageSensorPin = A6; 

const float ADC_SCALE = 1023.0;
const float VREF = 5.0;
const float VOLTAGE_SENSOR_SCALE = 100.0; 
const float CURRENT_SENSOR_SCALE = 30.0; 

const int numSamples = 200;
int voltageSamples[numSamples];
int currentSamples[numSamples];

void setup() {
  Serial.begin(9600);
}

void loop() {
  for (int i = 0; i < numSamples; i++) {
    voltageSamples[i] = analogRead(voltageSensorPin);
    currentSamples[i] = analogRead(currentSensorPin);
    delay(1); 
  }

  float voltageOffset = calculateMean(voltageSamples, numSamples) * (VREF / ADC_SCALE) * VOLTAGE_SENSOR_SCALE;
  float currentOffset = (calculateMean(currentSamples, numSamples) - 512) * (VREF / ADC_SCALE) * CURRENT_SENSOR_SCALE;

  float phaseDifference = calculatePhaseDifference(voltageSamples, currentSamples, numSamples, voltageOffset, currentOffset);
  float powerFactor = cos(phaseDifference);

  Serial.print("Phase Difference: ");
  Serial.print(phaseDifference * 180 / PI); 
  Serial.println(" degrees");
  Serial.print("Power Factor: ");
  Serial.println(powerFactor);

  delay(1000);
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
    float voltage = (voltageSamples[i] * (VREF / ADC_SCALE) * VOLTAGE_SENSOR_SCALE) - voltageOffset;
    float current = ((currentSamples[i] - 512) * (VREF / ADC_SCALE) * CURRENT_SENSOR_SCALE) - currentOffset;

    sumProduct += voltage * current;
    sumVoltageSquared += voltage * voltage;
    sumCurrentSquared += current * current;
  }

  return acos(sumProduct / sqrt(sumVoltageSquared * sumCurrentSquared));
}
