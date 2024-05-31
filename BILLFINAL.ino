#include <ZMPT101B.h>
#include <TimeLib.h> // Arduino Time 라이브러리 사용

const int ACPin = A5;
const int VPin = A6;

#define ACTectionRange 20
#define VREF 5.0

#define SENSITIVITY 302.5f
ZMPT101B voltageSensor(VPin, 60.0);

float readACCurrentValue() {
    float ACCurrentValue = 0;
    float peakVoltage = 0;
    float voltageVirtualValue = 0;

    for (int i = 0; i < 5; i++) {
        peakVoltage += analogRead(ACPin);
        delay(1);
    }

    peakVoltage = peakVoltage / 5;

    voltageVirtualValue = peakVoltage * 0.707;
    voltageVirtualValue = (voltageVirtualValue / 1024 * VREF) / 2;

    ACCurrentValue = voltageVirtualValue * ACTectionRange;

    if (ACCurrentValue < 0) {
        ACCurrentValue = 0;
    }

    return ACCurrentValue;
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
    Serial.println(" won");

    Serial.print("기후환경요금: ");
    Serial.print(int_usage_kwh);
    Serial.print(" * 9 = ");
    Serial.print(climate_environment_charge);
    Serial.println(" won");

    Serial.print("연료비조정요금: ");
    Serial.print(int_usage_kwh);
    Serial.print(" * 5 = ");
    Serial.print(fuel_adjustment_charge);
    Serial.println(" won");

    int total_electricity_charge = base_charge + charge + climate_environment_charge + fuel_adjustment_charge;

    Serial.print("전기요금계: ");
    Serial.print(total_electricity_charge);
    Serial.println(" won");

    Serial.print("부가가치세: ");
    Serial.print("round(");
    Serial.print(total_electricity_charge);
    Serial.print(" * 0.10) = ");
    Serial.print(vat);
    Serial.println(" won");

    Serial.print("전력기반기금: ");
    Serial.print("int(");
    Serial.print(total_electricity_charge);
    Serial.print(" * 0.037 / 10) * 10 = ");
    Serial.print(electric_fund);
    Serial.println(" won");

    Serial.print("청구금액: ");
    Serial.print(final_charge);
    Serial.println(" won");
}

int calculateElectricityCharge(int int_usage_kwh, bool isSummer) {
    int base_charge;
    int charge;

    // 기본 요금 계산
    if (int_usage_kwh <= 200) {
        base_charge = 910;
    } else if (int_usage_kwh <= 400) {
        base_charge = 1600;
    } else {
        base_charge = 7300;
    }

    // 전력 요금 계산
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

    // 기후환경요금 및 연료비조정요금 계산
    int climate_environment_charge = int_usage_kwh * 9;
    int fuel_adjustment_charge = int_usage_kwh * 5;

    // 총 전력 요금 계산
    int total_charge = charge + base_charge + climate_environment_charge + fuel_adjustment_charge;

    // 부가가치세 계산 (전력 요금의 10%, 반올림 후 정수로 변환)
    int vat = round(total_charge * 0.10);

    // 전력기반기금 계산 (전력 요금의 3.7%, 10원 미만 절사)
    int electric_fund = (total_charge * 0.037 / 10) * 10;

    // 최종 요금 계산
    int final_charge = total_charge + vat + electric_fund;

    // 계산 과정 출력
    printCalculationDetails(0.0, int_usage_kwh, isSummer, base_charge, charge, climate_environment_charge, fuel_adjustment_charge, vat, electric_fund, final_charge);

    return final_charge;
}

void setup() {
    Serial.begin(9600);
    pinMode(13, OUTPUT);
    voltageSensor.setSensitivity(SENSITIVITY);
}

void loop() {
    float ACCurrentValue = readACCurrentValue();
    float voltage = voltageSensor.getRmsVoltage();

    if (voltage < 0) {
        voltage = 0;
    }

    float power = ACCurrentValue * voltage;

    // 특정 시간 동안의 전력 사용량을 kWh로 변환 (10초 동안의 사용량을 1시간으로 환산)
    float usage_kwh = (power * 6 * 60) / 1000.0; // W를 kWh로 변환
    int int_usage_kwh = round(usage_kwh); // 올바르게 반올림하여 정수로 변환

    // 현재 날짜를 기반으로 계절 정보를 자동으로 설정
    time_t t = now();
    int currentMonth = month();
    bool isSummer = (currentMonth >= 7 && currentMonth <= 8); // 7월과 8월은 여름

    // 전력 요금 계산
    int final_charge = calculateElectricityCharge(int_usage_kwh, isSummer);

    // 결과 출력
    Serial.print("전류: ");
    Serial.print(ACCurrentValue, 2);
    Serial.println(" A");
    Serial.print("전압: ");
    Serial.print(voltage, 2);
    Serial.println(" V");
    Serial.print("실시간 전력: ");
    Serial.print(usage_kwh, 2);
    Serial.println(" kwh");
    Serial.print("계산 전력: ");
    Serial.print(int_usage_kwh);
    Serial.println(" kWh");
    Serial.print("현재 계절: ");
    Serial.println(isSummer ? "여름" : "기타계절");
    Serial.println();

    delay(2000); // 10초 대기
}
