#include <Arduino.h>
#include <math.h>
#define DUST_SENSOR_PIN D4
#define SENSOR_READING_INTERVAL 30000

unsigned long lastInterval;
unsigned long lowpulseoccupancy = 0;
unsigned long last_lpo = 0;
unsigned long duration;

float ratio = 0;
float concentration = 0;

void setup()
{
    Serial.begin(9600);

    pinMode(DUST_SENSOR_PIN, INPUT);
    lastInterval = millis();
}

void getDustSensorReadings()
{
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

    Serial.printlnf("LPO: %d", lowpulseoccupancy);
    Serial.printlnf("Ratio: %f%%", ratio);
    Serial.printlnf("Concentration: %f pcs/L", concentration);
}

void loop()
{
    duration = pulseIn(DUST_SENSOR_PIN, LOW);
    lowpulseoccupancy = lowpulseoccupancy + duration;

    if ((millis() - lastInterval) > SENSOR_READING_INTERVAL)
    {
        getDustSensorReadings();

        lowpulseoccupancy = 0;
        lastInterval = millis();
    }
}