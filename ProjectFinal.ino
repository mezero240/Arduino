#include <Adafruit_GFX.h>
#include <Adafruit_TFTLCD.h>
#include <TouchScreen.h>
#include <SD.h>
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
const int numSamples = 50; 
int voltageSamples[numSamples];
int currentSamples[numSamples];

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define SD_CS 10

#define BLACK   0x0000
#define WHITE   0xFFFF

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, A4);


void bmpDraw(char *filename, int x, int y);
uint16_t read16(File f);
uint32_t read32(File f);
float readACCurrentValue();
int calculateElectricityCharge(int int_usage_kwh, bool isSummer, int &base_charge, int &charge, int &climate_environment_charge, int &fuel_adjustment_charge, int &total_charge, int &vat, int &electric_fund);
float calculateMean(int *samples, int numSamples);
float calculatePhaseDifference(int *voltageSamples, int *currentSamples, int numSamples, float voltageOffset, float currentOffset);

const char str_sd_init_fail[] PROGMEM = "SD card initialization failed!";
const char str_file_not_found[] PROGMEM = "File not found";
const char str_phase_difference[] PROGMEM = "위상차: ";
const char str_power_factor[] PROGMEM = "역률: ";
const char str_current[] PROGMEM = "전류: ";
const char str_voltage[] PROGMEM = "전압: ";
const char str_estimated_power[] PROGMEM = "한달 예상 전력: ";
const char str_calculated_power[] PROGMEM = "계산 전력: ";
const char str_current_season[] PROGMEM = "현재 계절: ";
const char str_summer[] PROGMEM = "여름";
const char str_other_season[] PROGMEM = "기타계절";
const char str_base_charge[] PROGMEM = "기본요금: ";
const char str_electricity_charge[] PROGMEM = "전기요금: ";
const char str_climate_environment_charge[] PROGMEM = "기후환경요금: ";
const char str_fuel_adjustment_charge[] PROGMEM = "연료비조정요금: ";
const char str_total_electricity_charge[] PROGMEM = "전기요금계: ";
const char str_vat[] PROGMEM = "부가가치세: ";
const char str_electric_fund[] PROGMEM = "전력기반기금: ";
const char str_final_charge[] PROGMEM = "청구금액: ";

void setup() {
    Serial.begin(9600);
    tft.reset();
    delay(1000);
    uint16_t identifier = 0x9341;
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    tft.begin(identifier);
    if (!SD.begin(SD_CS)) {
        Serial.println((__FlashStringHelper*)str_sd_init_fail);
        return;
    }
    tft.setRotation(1);
    voltageSensor.setSensitivity(SENSITIVITY);
    pinMode(13, OUTPUT);

    tft.fillScreen(BLACK);
    bmpDraw("1.bmp", 0, 0);
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
    powerFactor = round(cos(phaseDifference) * 100) / 100.0;

    float ACCurrentValue = readACCurrentValue();
    ACCurrentValue = round(readACCurrentValue() * 100) / 100.0;

    float voltage = voltageSensor.getRmsVoltage();
    voltage = round(voltageSensor.getRmsVoltage() * 100) / 100.0;

    if (voltage < 0) {
        voltage = 0;
    }

    float power = ACCurrentValue * voltage * powerFactor;
    float usage_kwh = (power * 30 * 60) / 1000.0;
    if (usage_kwh < 1) {
        usage_kwh = 0;
    }
    int int_usage_kwh = round(usage_kwh); 

    time_t t = now();
    int currentMonth = month();
    bool isSummer = (currentMonth >= 7 && currentMonth <= 8);

    int base_charge, charge, climate_environment_charge, fuel_adjustment_charge, total_charge, vat, electric_fund;
    int final_charge = calculateElectricityCharge(int_usage_kwh, isSummer, base_charge, charge, climate_environment_charge, fuel_adjustment_charge, total_charge, vat, electric_fund);

    Serial.println();
    Serial.print(F("위상차: "));
    Serial.print(phaseDifference * 180 / PI);
    Serial.println(F(" degrees"));
    Serial.print(F("역률: "));
    Serial.println(powerFactor);
    Serial.print(F("전류: "));
    Serial.print(ACCurrentValue, 2);
    Serial.println(F(" A"));
    Serial.print(F("전압: "));
    Serial.print(voltage, 2);
    Serial.println(F(" V"));

    Serial.print(F("한달예상전력: "));
    Serial.print(power,2);
    Serial.print(F("*30*60/1000 = "));
    Serial.print(usage_kwh, 2);
    Serial.println(F(" kwh"));

    Serial.print(F("계산전력: "));
    Serial.print(int_usage_kwh);
    Serial.println(F(" kWh"));
    Serial.print(F("계절: "));
    Serial.println(isSummer ? (__FlashStringHelper*)str_summer : (__FlashStringHelper*)str_other_season);


    Serial.print(F("기본요금: ")); 
    Serial.print(base_charge);
    Serial.println(F(" 원"));

    Serial.print(F("전기요금: ")); 
     if (isSummer) {
        if (int_usage_kwh <= 300) {
            Serial.print(int_usage_kwh);
            Serial.print(F(" * 120 = "));
            Serial.print(charge);
        } else if (int_usage_kwh <= 450) {
            Serial.print(F("300 * 120 + ("));
            Serial.print(int_usage_kwh - 300);
            Serial.print(F(" * 214.6) = "));
            Serial.print(charge);
        } else {
            Serial.print(F("300 * 120 + 150 * 214.6 + ("));
            Serial.print(int_usage_kwh - 450);
            Serial.print(F(" * 307.3) = "));
            Serial.print(charge);
        }
    } else {
        if (int_usage_kwh <= 200) {
            Serial.print(int_usage_kwh);
            Serial.print(F(" * 120 = "));
            Serial.print(charge);
        } else if (int_usage_kwh <= 400) {
            Serial.print(F("200 * 120 + ("));
            Serial.print(int_usage_kwh - 200);
            Serial.print(F(" * 214.6) = "));
            Serial.print(charge);
        } else {
            Serial.print(F("200 * 120 + 200 * 214.6 + ("));
            Serial.print(int_usage_kwh - 400);
            Serial.print(F(" * 307.3) = "));
            Serial.print(charge);
        }
    }
    Serial.println(" 원");

    Serial.print(F("기후환경요금: ")); 
    Serial.print(int_usage_kwh);
    Serial.print(F(" * 9 = "));
    Serial.print(climate_environment_charge);
    Serial.println(F(" 원"));

    Serial.print(F("연료비조정요금: ")); 
    Serial.print(int_usage_kwh);
    Serial.print(" * 5 = ");
    Serial.print(fuel_adjustment_charge); 
    Serial.println(F(" 원"));

    int total_electricity_charge = base_charge + charge + climate_environment_charge + fuel_adjustment_charge;
    
   Serial.print(F("전기요금계: ")); 
    Serial.print(total_electricity_charge);
    Serial.println(F(" 원"));

    Serial.print(F("부가가치세: ")); 
    Serial.print(F("round("));
    Serial.print(total_electricity_charge);
    Serial.print(F(" * 0.10) = "));
    Serial.print(vat);
    Serial.println(F(" 원"));

    Serial.print(F("전력기반기금: ")); 
    Serial.print(F("int("));
    Serial.print(total_electricity_charge);
    Serial.print(F(" * 0.037 / 10) * 10 = "));
    Serial.print(electric_fund);
    Serial.println(F(" 원"));

    Serial.print(F("청구금액: ")); 
    Serial.print(final_charge);
    Serial.println(F(" 원"));

    //TFT
    tft.fillRect(30, 70, 280, 30, WHITE);
    tft.setCursor(30, 70);
    tft.setTextColor(BLACK);
    tft.setTextSize(3);
    tft.print(usage_kwh);
    tft.print("kWh");

    tft.fillRect(30, 180, 280, 30, WHITE);
    tft.setCursor(30, 180);
    tft.setTextColor(BLACK);
    tft.setTextSize(3);
    tft.print(final_charge);
    tft.print("WON");

    delay(1000);
    
}

#define BUFFPIXEL 20

void bmpDraw(char *filename, int x, int y) {
    File bmpFile;
    int bmpWidth, bmpHeight;
    uint8_t bmpDepth;
    uint32_t bmpImageoffset, rowSize;
    uint8_t sdbuffer[3 * BUFFPIXEL];
    uint16_t lcdbuffer[BUFFPIXEL];
    uint8_t buffidx = sizeof(sdbuffer);
    boolean goodBmp = false, flip = true;
    int w, h, row, col;
    uint8_t r, g, b;
    uint32_t pos = 0, startTime = millis();
    uint8_t lcdidx = 0;
    boolean first = true;

    if ((x >= tft.width()) || (y >= tft.height())) return;

    if ((bmpFile = SD.open(filename)) == NULL) {
        Serial.print((__FlashStringHelper*)str_file_not_found);
        return;
    }

    if (read16(bmpFile) == 0x4D42) {
        read32(bmpFile);
        read32(bmpFile);
        bmpImageoffset = read32(bmpFile);
        read32(bmpFile);
        bmpWidth = read32(bmpFile);
        bmpHeight = read32(bmpFile);

        if (read16(bmpFile) == 1) {
            bmpDepth = read16(bmpFile);

            if ((bmpDepth == 24) && (read32(bmpFile) == 0)) {
                goodBmp = true;

                rowSize = (bmpWidth * 3 + 3) & ~3;

                if (bmpHeight < 0) {
                    bmpHeight = -bmpHeight;
                    flip = false;
                }

                w = bmpWidth;
                h = bmpHeight;
                if ((x + w - 1) >= tft.width()) w = tft.width() - x;
                if ((y + h - 1) >= tft.height()) h = tft.height() - y;

                tft.setAddrWindow(0, 0, 319, 239);

                for (row = 0; row < h; row++) {
                    pos = flip ? bmpImageoffset + (bmpHeight - 1 - row) * rowSize : bmpImageoffset + row * rowSize;
                    if (bmpFile.position() != pos) {
                        bmpFile.seek(pos);
                        buffidx = sizeof(sdbuffer);
                    }

                    for (col = 0; col < w; col++) {
                        if (buffidx >= sizeof(sdbuffer)) {
                            if (lcdidx > 0) {
                                tft.pushColors(lcdbuffer, lcdidx, first);
                                lcdidx = 0;
                                first = false;
                            }
                            bmpFile.read(sdbuffer, sizeof(sdbuffer));
                            buffidx = 0;
                        }

                        b = sdbuffer[buffidx++];
                        g = sdbuffer[buffidx++];
                        r = sdbuffer[buffidx++];
                        lcdbuffer[lcdidx++] = tft.color565(r, g, b);
                    }
                }

                if (lcdidx > 0) {
                    tft.pushColors(lcdbuffer, lcdidx, first);
                }
            }
        }
    }

    bmpFile.close();
}

uint16_t read16(File f) {
    uint16_t result;
    ((uint8_t*)&result)[0] = f.read();
    ((uint8_t*)&result)[1] = f.read();
    return result;
}

uint32_t read32(File f) {
    uint32_t result;
    ((uint8_t*)&result)[0] = f.read();
    ((uint8_t*)&result)[1] = f.read();
    ((uint8_t*)&result)[2] = f.read();
    ((uint8_t*)&result)[3] = f.read();
    return result;
}

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

int calculateElectricityCharge(int int_usage_kwh, bool isSummer, int &base_charge, int &charge, int &climate_environment_charge, int &fuel_adjustment_charge, int &total_charge, int &vat, int &electric_fund) {
    if (int_usage_kwh < 1) {
        base_charge = 0;
        charge = 0;
        climate_environment_charge = 0;
        fuel_adjustment_charge = 0;
        total_charge = 0;
        vat = 0;
        electric_fund = 0;
        return 0;
    }

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

    climate_environment_charge = int_usage_kwh * 9;
    fuel_adjustment_charge = int_usage_kwh * 5;

    total_charge = charge + base_charge + climate_environment_charge + fuel_adjustment_charge;

    vat = round(total_charge * 0.10);

    electric_fund = (total_charge * 0.037 / 10) * 10;

    return total_charge + vat + electric_fund;
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
