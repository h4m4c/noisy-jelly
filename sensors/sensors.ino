#include <Wire.h>
#include <CapacitiveSensor.h>

#define MIN_SENSE1 700
#define MIN_SENSE2 1100

// 100K resistor between pins 4 & 2
CapacitiveSensor   cs_4_2 = CapacitiveSensor(4,2);
long min1 = 0;
long max1 = 0;
unsigned char score1 = 0;

// 100K resistor between pins 4 & 6
CapacitiveSensor   cs_4_6 = CapacitiveSensor(4,6);
long min2 = 0;
long max2 = 0;
unsigned char score2 = 0;

#define LED_PIN 13

void setup()                    
{
   Serial.begin(9600);
   
   Wire.begin(8);
   Wire.onRequest(requestData);

   pinMode(LED_PIN, OUTPUT);
}

void loop()                    
{
    long total1 =  cs_4_2.capacitiveSensorRaw(80);
    long total2 =  cs_4_6.capacitiveSensorRaw(80);

    if (total1 < MIN_SENSE1) {
        min1 = 2147483647;
        max1 = 0;
        score1 = 0;
    } else {
        min1 = min(min1, total1);
        max1 = max(max1, total1);
        score1 = (total1 - min1)*255/(max1 - min1);
    }

    if (total2 < MIN_SENSE2) {
        min2 = 2147483647;
        max2 = 0;
        score2 = 0;
    } else {
        min2 = min(min2, total2);
        max2 = max(max2, total2);
        score2 = (total2 - min2)*255/(max2 - min2);
    }

    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    Serial.print(score1); 
    Serial.print("\t");
    Serial.print(score2);
    Serial.print("\t");
    Serial.print(total1);
    Serial.print("\t");
    Serial.println(total2);

    delay(10);
}

void requestData() {
  Wire.write(score1);
  Wire.write(score2);
}
