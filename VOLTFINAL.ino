#include <ZMPT101B.h>

#define SENSITIVITY 302.5f 
ZMPT101B voltageSensor(A6, 60.0);

void setup() {
  Serial.begin(9600);
  voltageSensor.setSensitivity(SENSITIVITY);
}

void loop() {
  float voltage = voltageSensor.getRmsVoltage();
  Serial.println(voltage);
  delay(1000);
}