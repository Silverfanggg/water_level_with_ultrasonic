#include <Arduino.h>

const int trigPin = 5;
const int echoPin = 18;

const float sensor_height = 200.0;
const float glass_height = 104.0;
const float sound_speed = 0.343; 

const int NUM_SAMPLES = 15;
const int MEASUREMENT_INTERVAL_MS = 200;

enum MeasurementState {
  IDLE,
  MEASURING,
  READY
};

struct {
  MeasurementState state = IDLE;
  float measurements[NUM_SAMPLES];
  int sampleCount = 0;
  unsigned long lastMeasureTime = 0;
  unsigned long lastDisplayTime = 0;
  float lastValidAvg = 96.0;
  int measurementErrors = 0;
} ctx;

float measureDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if (duration == 0)
    return -1.0;

  float distance_mm = (duration / 2.0) * sound_speed;
  
  if (distance_mm > 300 || distance_mm < 50)
    return -1.0;
  
  return distance_mm;
}

void startMeasurement()
{
  ctx.state = MEASURING;
  ctx.sampleCount = 0;
  ctx.lastMeasureTime = millis();
  ctx.measurementErrors = 0;
  
  for (int i = 0; i < NUM_SAMPLES; i++)
    ctx.measurements[i] = -1;
}

void processMeasurement()
{
  unsigned long currentTime = millis();

  if (currentTime - ctx.lastMeasureTime >= MEASUREMENT_INTERVAL_MS)
  {
    ctx.lastMeasureTime = currentTime;

    float d = measureDistance();
    if (d > 0)
    {
      ctx.measurements[ctx.sampleCount] = d;
      Serial.print(".");
    }
    else
    {
      ctx.measurements[ctx.sampleCount] = -1;
      ctx.measurementErrors++;
      Serial.print("x");
    }

    ctx.sampleCount++;

    if (ctx.sampleCount >= NUM_SAMPLES)
    {
      float total = 0;
      int validCount = 0;
      float minVal = 999;
      float maxVal = 0;

      for (int i = 0; i < NUM_SAMPLES; i++)
      {
        if (ctx.measurements[i] > 0)
        {
          total += ctx.measurements[i];
          validCount++;
          if (ctx.measurements[i] < minVal) minVal = ctx.measurements[i];
          if (ctx.measurements[i] > maxVal) maxVal = ctx.measurements[i];
        }
      }

      Serial.println();
      
      if (validCount >= (NUM_SAMPLES * 0.8))
      {
        ctx.lastValidAvg = total / validCount;
        Serial.print("OK: ");
        Serial.print(validCount);
        Serial.print("/");
        Serial.print(NUM_SAMPLES);
        Serial.print(" | Range: ");
        Serial.print(minVal, 1);
        Serial.print("-");
        Serial.print(maxVal, 1);
        Serial.print("mm | Avg: ");
        Serial.print(ctx.lastValidAvg, 1);
        Serial.println("mm");
      }
      else
      {
        Serial.print("FAIL: Only ");
        Serial.print(validCount);
        Serial.print("/");
        Serial.println(NUM_SAMPLES);
      }

      ctx.state = READY;
    }
  }
}

int getWaterLevel()
{
  if (ctx.lastValidAvg < 0)
    return -1;

  float water_level = (sensor_height - ctx.lastValidAvg) / glass_height;
  int percent = int(water_level * 100);
  
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  
  return percent;
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  Serial.println("\n=== Water Level Sensor ===");
  Serial.print("Samples: ");
  Serial.print(NUM_SAMPLES);
  Serial.print(" | Interval: ");
  Serial.print(MEASUREMENT_INTERVAL_MS);
  Serial.println("ms");
  
  startMeasurement();
}

void loop()
{
  unsigned long currentTime = millis();

  if (ctx.state == MEASURING)
  {
    processMeasurement();
  }

  if (currentTime - ctx.lastDisplayTime >= 5000)
  {
    ctx.lastDisplayTime = currentTime;

    int waterLevel = getWaterLevel();
    Serial.print(">>> Water Level: ");
    Serial.print(waterLevel);
    Serial.println("% <<<");

    startMeasurement();
  }

}


