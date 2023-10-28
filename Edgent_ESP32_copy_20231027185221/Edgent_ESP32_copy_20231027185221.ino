#include <Wire.h>
#include <time.h>
#include <RTClib.h>

#define BLYNK_TEMPLATE_ID "TMPL2IVCyko9U"
#define BLYNK_TEMPLATE_NAME "Pet Feeder"

RTC_DS3231 rtc;

const int PowerLed = 4;  // Pin connected to the LED Power Led
const int WifiLed = 15; //Pin connected to the LED Wifi Led
const int MotorDuration = 60000;  // LED on duration in milliseconds (60 seconds)

struct Timer {
  uint8_t hours;
  uint8_t minutes;
};

Timer timer1 = { 8, 30 };  // Timer 1 set to 8:20 pm
Timer timer2 = { 12, 0 };   // Timer 2 set to 12:00 pm
Timer timer3 = { 16, 0 };   // Timer 3 set to 4:00 pm

bool MotorActive = false;  // Track if the LED is currently active
int OneCycle = 0;
//28BYJ 5V or 12V stepper Motor Connection
const int STEPPER_PIN_1 = 23;
const int STEPPER_PIN_2 = 19;
const int STEPPER_PIN_3 = 18;
const int STEPPER_PIN_4 = 5;

const int STEPS_PER_REVOLUTION = 2048;
const float DEGREES_PER_STEP = 360.0 / STEPS_PER_REVOLUTION;
const int TARGET_DEGREES = 180;

#define BLYNK_FIRMWARE_VERSION "0.1.0"

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

#define APP_DEBUG

#define USE_ESP32_DEV_MODULE

#include "BlynkEdgent.h"
void setup() {
  Serial.begin(115200);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }


  pinMode(STEPPER_PIN_1, OUTPUT);
  pinMode(STEPPER_PIN_2, OUTPUT);
  pinMode(STEPPER_PIN_3, OUTPUT);
  pinMode(STEPPER_PIN_4, OUTPUT);

  pinMode(PowerLed, OUTPUT);
  pinMode(WifiLed, OUTPUT);

  digitalWrite(PowerLed, HIGH);
  digitalWrite(WifiLed, LOW);

  delay(100);

  BlynkEdgent.begin();
}

void loop() {
  BlynkEdgent.run();

  DateTime now = rtc.now();

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(WifiLed, HIGH); // Turn on WifiLed
    syncRTC();
  } else {
    digitalWrite(WifiLed, LOW); // Turn off WifiLed
  }
  

  Serial.print("Current Time: ");
  printTime(now);

  checkAndActivateTimer(now, timer1);
  checkAndActivateTimer(now, timer2);
  checkAndActivateTimer(now, timer3);

  delay(1000);  // Add a delay to avoid flooding the Serial Monitor
}
BLYNK_WRITE(V0) {
  int pinValue = param.asInt();
  if (pinValue == 1 && OneCycle == 0) {
    rotateClockwise();
    OneCycle = 1;
  } else if (pinValue == 0) {
    OneCycle = 0;
  }
  Serial.println(pinValue);
  delay(1000);
}
void printTime(DateTime time) {
  Serial.print(time.year(), DEC);
  Serial.print('/');
  Serial.print(time.month(), DEC);
  Serial.print('/');
  Serial.print(time.day(), DEC);
  Serial.print(" ");
  Serial.print(time.hour(), DEC);
  Serial.print(':');
  Serial.print(time.minute(), DEC);
  Serial.print(':');
  Serial.print(time.second(), DEC);
  Serial.println();
}
void checkAndActivateTimer(DateTime now, Timer timer) {
  if (OneCycle == 0) {
    if (now.hour() == timer.hours && now.minute() == timer.minutes && !MotorActive) {
      Serial.println("Timer Activated!");
      rotateClockwise();
      MotorActive = true;
      OneCycle = 1;
      delay(5000);
    }
  }else{
    OneCycle = 0;
  }
}
void rotateClockwise() {
  int targetSteps = TARGET_DEGREES / DEGREES_PER_STEP;
  const int stepSequence[8][4] = {
    { HIGH, LOW, LOW, LOW },
    { HIGH, HIGH, LOW, LOW },
    { LOW, HIGH, LOW, LOW },
    { LOW, HIGH, HIGH, LOW },
    { LOW, LOW, HIGH, LOW },
    { LOW, LOW, HIGH, HIGH },
    { LOW, LOW, LOW, HIGH },
    { HIGH, LOW, LOW, HIGH }
  };
  for (int i = 0; i < targetSteps; i++) {
    for (int j = 0; j < 8; j++) {
      digitalWrite(STEPPER_PIN_1, stepSequence[j][0]);
      digitalWrite(STEPPER_PIN_2, stepSequence[j][1]);
      digitalWrite(STEPPER_PIN_3, stepSequence[j][2]);
      digitalWrite(STEPPER_PIN_4, stepSequence[j][3]);
      delayMicroseconds(1000);
    }
  }
  digitalWrite(STEPPER_PIN_1, LOW);
  digitalWrite(STEPPER_PIN_2, LOW);
  digitalWrite(STEPPER_PIN_3, LOW);
  digitalWrite(STEPPER_PIN_4, LOW);
}
void syncRTC() {
  // Set timezone offset for Nairobi (East Africa Time, EAT)
  const long timezoneOffset = -3 * 3600; // 3 hours in seconds

  configTime(timezoneOffset, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for time sync");
  while (time(nullptr) < timezoneOffset) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("");
  Serial.println("Time synchronized with NTP");
  rtc.adjust(DateTime(time(nullptr) - timezoneOffset));
}

