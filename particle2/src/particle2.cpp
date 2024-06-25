#include <Arduino.h>
#include <math.h>

//air quality
#include "Air_Quality_Sensor.h"

//dust
#define DUST_SENSOR_PIN D4
#define SENSOR_READING_INTERVAL 30000

//dust
unsigned long lastInterval;
unsigned long lowpulseoccupancy = 0;
unsigned long last_lpo = 0;
unsigned long duration;
//dust
float ratio = 0;
float concentration = 0;

//air quality
AirQualitySensor sensor(A0);

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
    //dust
    duration = pulseIn(DUST_SENSOR_PIN, LOW);
    lowpulseoccupancy = lowpulseoccupancy + duration;

    if ((millis() - lastInterval) > SENSOR_READING_INTERVAL)
    {
        getDustSensorReadings();
        getAirQualityReadings();

        lowpulseoccupancy = 0;
        lastInterval = millis();
    }
}