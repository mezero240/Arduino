#include <ZMPT101B.h>
#include <TimeLib.h>
#include <math.h>

const int VPin = A6;
const int analogInPin = A5;

#define ACTectionRange 20
#define VREF_VALUE 5.0

#define SENSITIVITY 302.5f
ZMPT101B voltageSensor(VPin, 60.0);

const int currentSensorPin = A5; 
const int voltageSensorPin = A6; 

const float ADC_SCALE_VALUE = 1023.0;
const float VOLTAGE_SENSOR_SCALE = 100.0; 
const float CURRENT_SENSOR_SCALE = 30.0; 

const int numSamples = 200;
int voltageSamples[numSamples];
int currentSamples[numSamples];

float readACCurrentValue() {
    float voltage;
    float currentAC;
    float maxVoltage = 0;
    float minVoltage = 5;

    for (int i = 0; i < 1000; i++) {
        float sensorValue = analogRead(analogInPin);
        voltage = (sensorValue / 1023.0) * 5.0;
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

    return currentAC / 1000.0;
}

void printCalculationDetails(float usage_kwh, int int_usage_kwh, bool isSummer, int base_charge, int charge, int climate_environment_charge, int fuel_adjustment_charge, int vat, int electric_fund, int final_charge) {
    Serial.print("기본요금: ");
    Serial.print(base_charge);
    Serial.println(" won");

    Serial.print("전기요금: ");
     if (isSummer) {
        if (int_usage_kwh <= 300) {
            Serial.print(int_usage_kwh);
            Serial.print(" * 120 = ");
            Serial.print(charge);
        } else if (int_usage_kwh <= 450) {
            Serial.print("300 * 120 + (");
            Serial.print(int_usage_kwh - 300);
            Serial.print(" * 214.6) = ");
            Serial.print(charge);
        } else {
            Serial.print("300 * 120 + 150 * 214.6 + (");
            Serial.print(int_usage_kwh - 450);
            Serial.print(" * 307.3) = ");
            Serial.print(charge);
        }
    } else {
        if (int_usage_kwh <= 200) {
            Serial.print(int_usage_kwh);
            Serial.print(" * 120 = ");
            Serial.print(charge);
        } else if (int_usage_kwh <= 400) {
            Serial.print("200 * 120 + (");
            Serial.print(int_usage_kwh - 200);
            Serial.print(" * 214.6) = ");
            Serial.print(charge);
        } else {
            Serial.print("200 * 120 + 200 * 214.6 + (");
            Serial.print(int_usage_kwh - 400);
            Serial.print(" * 307.3) = ");
            Serial.print(charge);
        }
    }
    Serial.println(" 원");

    Serial.print("기후환경요금: ");
    Serial.print(int_usage_kwh);
    Serial.print(" * 9 = ");
    Serial.print(climate_environment_charge);
    Serial.println(" 원");total_charge

    Serial.print("연료비조정요금: ");
    Serial.print(int_usage_kwh);
    Serial.print(" * 5 = ");
    Serial.print(fuel_adjustment_charge);
    Serial.println(" 원");

    int total_electricity_charge = base_charge + charge + climate_environment_charge + fuel_adjustment_charge;

    Serial.print("전기요금계: ");
    Serial.print(total_electricity_charge);
    Serial.println(" 원");

    Serial.print("부가가치세: ");
    Serial.print("round(");
    Serial.print(total_electricity_charge);
    Serial.print(" * 0.10) = ");
    Serial.print(vat);
    Serial.println(" 원");

    Serial.print("전력기반기금: ");
    Serial.print("int(");
    Serial.print(total_electricity_charge);
    Serial.print(" * 0.037 / 10) * 10 = ");
    Serial.print(electric_fund);
    Serial.println(" 원");

    Serial.print("청구금액: ");
    Serial.print(final_charge);
    Serial.println(" 원");
}

int calculateElectricityCharge(int int_usage_kwh, bool isSummer) {
    if (int_usage_kwh < 1) {
        printCalculationDetails(0.0, int_usage_kwh, isSummer, 0, 0, 0, 0, 0, 0, 0);
        return 0;
    }

    int base_charge;
    int charge;

    if (int_usage_kwh <= 200) {
        base_charge = 910;
    } else if (int_usage_kwh <= 400) {
        base_charge = 1600;
    } else {
        base_charge = 7300;
    }

    if (isSummer) {
        if (int_usage_kwh <= 300) {
            charge = int_usage_kwh * 120;
        } else if (int_usage_kwh <= 450) {
            charge = 300 * 120 + (int_usage_kwh - 300) * 214.6;
        } else {
            charge = 300 * 120 + 150 * 214.6 + (int_usage_kwh - 450) * 307.3;
        }
    } else {
        if (int_usage_kwh <= 200) {
            charge = int_usage_kwh * 120;
        } else if (int_usage_kwh <= 400) {
            charge = 200 * 120 + (int_usage_kwh - 200) * 214.6;
        } else {
            charge = 200 * 120 + 200 * 214.6 + (int_usage_kwh - 400) * 307.3;
        }
    }

    int climate_environment_charge = int_usage_kwh * 9;
    int fuel_adjustment_charge = int_usage_kwh * 5;
    int total_charge = charge + base_charge + climate_environment_charge + fuel_adjustment_charge;
    int vat = round(total_charge * 0.10);
    int electric_fund = (total_charge * 0.037 / 10) * 10;
    int final_charge = total_charge + vat + electric_fund;

    printCalculationDetails(0.0, int_usage_kwh, isSummer, base_charge, charge, climate_environment_charge, fuel_adjustment_charge, vat, electric_fund, final_charge);

    return final_charge;
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

void setup() {
    Serial.begin(9600);
    pinMode(13, OUTPUT);
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

    if (voltage < 0) {
        voltage = 0;
    }

    float power = ACCurrentValue * voltage * powerFactor;

    float usage_kwh = (power * 30 * 60) / 1000.0;
    int int_usage_kwh = round(usage_kwh);

    time_t t = now();
    int currentMonth = month();
    bool isSummer = (currentMonth >= 7 && currentMonth <= 8);

    int final_charge = calculateElectricityCharge(int_usage_kwh, isSummer);

    Serial.print("위상차: ");
    Serial.print(phaseDifference * 180 / PI); 
    Serial.println(" 도");
    Serial.print("역률: ");
    Serial.println(powerFactor);
    
    Serial.print("전류: ");
    Serial.print(ACCurrentValue, 2);
    Serial.println(" A");
    Serial.print("전압: ");
    Serial.print(voltage, 2);
    Serial.println(" V");
    Serial.print("한달 예상 전력: ");
    Serial.print(usage_kwh, 2);
    Serial.println(" kwh");
    Serial.print("계산 전력: ");
    Serial.print(int_usage_kwh);
    Serial.println(" kWh");
    Serial.print("현재 계절: ");
    Serial.println(isSummer ? "여름" : "기타계절");
    Serial.println();

    delay(2000);
}
