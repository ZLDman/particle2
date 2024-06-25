#include <Arduino.h>
#include <math.h>

//gas
#include <Multichannel_Gas_GMXXX.h>
#include <Wire.h>
GAS_GMXXX<TwoWire> gas;

//air quality
#include "Air_Quality_Sensor.h"

//VOC gas
#include <SensirionI2CSgp40.h>

//dust
#define DUST_SENSOR_PIN D4
#define SENSOR_READING_INTERVAL 30000

//dust
unsigned long lastInterval;
unsigned long lowpulseoccupancy = 0;
unsigned long last_lpo = 0;
unsigned long duration;
float ratio = 0;
float concentration = 0;

//air quality
AirQualitySensor sensor(A0);

//VOC gas
SensirionI2CSgp40 sgp40;

void setup()
{
    Serial.begin(9600);

    //dust
    pinMode(DUST_SENSOR_PIN, INPUT);
    lastInterval = millis();

    //air quality
    if (sensor.init()) {
        Serial.println("air quality Sensor ready.");
    } else {
        Serial.println("air quality Sensor ERROR!");
    }

    //gas
    gas.begin(Wire, 0x08);

    //VOC gas
    Wire.begin();

    uint16_t error;
    char errorMessage[256];

    sgp40.begin(Wire);

    uint16_t serialNumber[3];
    uint8_t serialNumberSize = 3;

    error = sgp40.getSerialNumber(serialNumber, serialNumberSize);

    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("SerialNumber:");
        Serial.print("0x");
        for (size_t i = 0; i < serialNumberSize; i++) {
            uint16_t value = serialNumber[i];
            Serial.print(value < 4096 ? "0" : "");
            Serial.print(value < 256 ? "0" : "");
            Serial.print(value < 16 ? "0" : "");
            Serial.print(value, HEX);
        }
        Serial.println();
    }

    uint16_t testResult;
    error = sgp40.executeSelfTest(testResult);
    if (error) {
        Serial.print("Error trying to execute executeSelfTest(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (testResult != 0xD400) {
        Serial.print("executeSelfTest failed with error: ");
        Serial.println(testResult);
    }
}

void getVOCgasReading(){
    uint16_t error;
    char errorMessage[256];
    uint16_t defaultRh = 0x8000;
    uint16_t defaultT = 0x6666;
    uint16_t srawVoc = 0;

    delay(1000);

    error = sgp40.measureRawSignal(defaultRh, defaultT, srawVoc);
    if (error) {
        Serial.print("Error trying to execute measureRawSignal(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("SRAW_VOC:");
        Serial.println(srawVoc);
    }
}

void getGasReading(){
    uint32_t val = 0;

    val = gas.getGM102B();
    Serial.print("GM102B carbon monoxide (CO): ");
    Serial.print(val);
    Serial.print("  =  ");
    Serial.print(gas.calcVol(val));
    Serial.println("V");
    val = gas.getGM302B();
    Serial.print("GM302B ammonia (NH3): ");
    Serial.print(val);
    Serial.print("  =  ");
    Serial.print(gas.calcVol(val));
    Serial.println("V");
    val = gas.getGM502B();
    Serial.print("GM502B nitrogen dioxide (NO2): ");
    Serial.print(val);
    Serial.print("  =  ");
    Serial.print(gas.calcVol(val));
    Serial.println("V");
    val = gas.getGM702B();
    Serial.print("GM702B volatile organic compounds (VOCs): ");
    Serial.print(val);
    Serial.print("  =  ");
    Serial.print(gas.calcVol(val));
    Serial.println("V");
}

void getAirQualityReadings(){
    int quality = sensor.slope();

    Serial.print("Sensor value: ");
    Serial.println(sensor.getValue());

    if (quality == AirQualitySensor::FORCE_SIGNAL) {
        Serial.println("High pollution! Force signal active.");
    } else if (quality == AirQualitySensor::HIGH_POLLUTION) {
        Serial.println("High pollution!");
    } else if (quality == AirQualitySensor::LOW_POLLUTION) {
        Serial.println("Low pollution!");
    } else if (quality == AirQualitySensor::FRESH_AIR) {
        Serial.println("Fresh air.");
    }
}

void getDustSensorReadings()
{
    //dust
    if (lowpulseoccupancy == 0)
    {
        lowpulseoccupancy = last_lpo;
    }
    else
    {
        last_lpo = lowpulseoccupancy;
    }

    ratio = lowpulseoccupancy / (SENSOR_READING_INTERVAL * 10.0);
    concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;

    Serial.printlnf("dust:");
    Serial.printlnf("LPO: %d", lowpulseoccupancy);
    Serial.printlnf("Ratio: %f%%", ratio);
    Serial.printlnf("Concentration: %f pcs/L", concentration);
}

void loop()
{
    //gas
    

    //dust
    duration = pulseIn(DUST_SENSOR_PIN, LOW);
    lowpulseoccupancy = lowpulseoccupancy + duration;

    if ((millis() - lastInterval) > SENSOR_READING_INTERVAL)
    {
        getDustSensorReadings();
        getAirQualityReadings();
        getGasReading();
        getVOCgasReading();

        lowpulseoccupancy = 0;
        lastInterval = millis();
    }
}