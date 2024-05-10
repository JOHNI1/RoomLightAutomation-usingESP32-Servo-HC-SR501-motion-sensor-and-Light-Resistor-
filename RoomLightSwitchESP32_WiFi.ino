/*************************************************************
  Blynk is a platform with iOS and Android apps to control
  ESP32, Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build mobile and web interfaces for any
  projects by simply dragging and dropping widgets.
/*************************************************************
  Blynk is a platform with iOS and Android apps to control
  ESP32, Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build mobile and web interfaces for any
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: https://www.blynk.io
    Sketch generator:           https://examples.blynk.cc
    Blynk community:            https://community.blynk.cc
    Follow us:                  https://www.fb.com/blynkapp
                                https://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on ESP32 chip.

  NOTE: This requires ESP32 support package:
    https://github.com/espressif/arduino-esp32

  Please be sure to select the right ESP32 module
  in the Tools -> Board menu!

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */
// #define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL6TCXFG3W3"
#define BLYNK_TEMPLATE_NAME "Switch"
#define BLYNK_AUTH_TOKEN "Ia6NTh9JH3kUyqYA2zZwyv1gWAIpbK3t"

#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <time.h>

WidgetRTC rtc;


// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Home";
char pass[] = "Mita63249484";

#define servoPin 19
#define HC_SR501 13
#define LightResistor 32

const int LightBorder = 1400;
bool IsLightOn = true;
time_t LightTimerStartTime;
const int LightRegisterTime = 3;  //in seconds and not millis!!
bool LightTimerStarted = false;

time_t MotionTimerStartTime;
const int MotionRegisterTime = 600;   //ten minutes
// const int MotionRegisterTime = 10;  //for expiriment, 10 sec
bool MotionTimerStarted = false;

time_t autoSwitchActionStartTime;
const int autoSwitchActionValidTimeWidth = 2; //if within 2 secs after the light turns on the user turns the light back off, it means that light should be kept off.
int lightIsReallyOffValidationCounter = 0;
const int lightIsReallyOffValidationCount = 7;    //to make sure that for seven frames, the light registered off!
bool autoLightOn = true;
bool afterLightOn = false;

int dayStartHour = 8;
int dayEndHour = 23;


time_t now_;
bool motionReading;
int lightReading;

bool IsItDay() {
  if (dayStartHour < dayEndHour) {
    return (dayStartHour < hour() && hour() < dayEndHour);
  } else {
    return (!(dayEndHour < hour() && hour() < dayStartHour));
  }
}

BLYNK_CONNECTED() {
  // Synchronize time on connection
  rtc.begin();
}

Servo myServo;
BLYNK_WRITE(V0) {
  int pinValue = param.asInt();  // assigning incoming value from pin V0 to a variable
  if (pinValue == 1) {
    turnOnLight();
  } else {
    turnOffLight();
  }
}
void turnOnLight() {
  myServo.attach(servoPin);
  myServo.write(63);  // turn servo to 63 degree if switch is ON
  delay(300);
  myServo.write(35);
  delay(400);
  myServo.detach();
}
void turnOffLight() {
  myServo.attach(servoPin);
  myServo.write(8);  // turn servo to 8 degree if switch is OFF
  delay(300);
  myServo.write(35);
  delay(400);
  myServo.detach();
}

void setup() {
  // Serial.begin(9600);
  pinMode(HC_SR501, INPUT);
  myServo.attach(servoPin);
  myServo.write(35);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  // setSyncInterval(60 * 60 * 24);  // Sync interval in seconds (every day)
  delay(700);
  myServo.detach();
}
void loop() {

  now_ = now();
  motionReading = digitalRead(HC_SR501);
  lightReading = analogRead(LightResistor);
  if (now_ > 1714405458) {

    if (!IsLightOn == (lightReading > LightBorder)) {
      if (LightTimerStarted == false) {
        LightTimerStartTime = now_;
        LightTimerStarted = true;
      } else if (now_ - LightTimerStartTime > LightRegisterTime) {
        IsLightOn = !IsLightOn;
        LightTimerStarted = false;
        autoSwitchActionStartTime = now_;
      }
    } else {
      LightTimerStarted = false;
    }




    if (autoLightOn) {
      //if its night, only automatically turn off and if day, turn off and on automatically.
      if (IsItDay() && !IsLightOn && !LightTimerStarted && motionReading) {
        IsLightOn = true;
        turnOnLight();
        autoSwitchActionStartTime = now_;
        lightIsReallyOffValidationCounter = 0;
        afterLightOn = true;
      } else if (IsLightOn && !LightTimerStarted && !motionReading) {
        if (MotionTimerStarted == false) {
          MotionTimerStartTime = now_;
          MotionTimerStarted = true;
        } else if (now_ - MotionTimerStartTime >= MotionRegisterTime) {
          turnOffLight();
          // Serial.println("lightoff!");
          IsLightOn = false;
          MotionTimerStarted = false;
        }
      }
    }
    if (motionReading) {
      MotionTimerStarted = false;
    }




    if (afterLightOn) {
      if (now_ - autoSwitchActionStartTime < autoSwitchActionValidTimeWidth && lightReading < LightBorder) {    //analogRead(LightResistor) < LightBorder means that its dark in the room
        if (lightIsReallyOffValidationCounter < lightIsReallyOffValidationCount) {
          lightIsReallyOffValidationCounter++;
          // Serial.println(lightIsReallyOffValidationCounter);
        } else {
          autoLightOn = false;
          afterLightOn = false;
          lightIsReallyOffValidationCounter = 0;
          // Serial.println("ligh off disabled!");
        }
      } else if (now_ - autoSwitchActionStartTime > autoSwitchActionValidTimeWidth) {
        afterLightOn = false;
        lightIsReallyOffValidationCounter = 0;
      }
    }
    if (lightReading > LightBorder) {
      autoLightOn = true;
      afterLightOn = false;
      lightIsReallyOffValidationCounter = 0;
      // Serial.println("light on enabled!");
    }
    // String currentTime = String(hour()) + ":" + minute() + ":" + second();
    // String currentDate = String(day()) + " " + month() + " " + year();
    // Serial.print("Current time: ");
    // Serial.print(currentTime);
    // Serial.print(" ");
    // Serial.print(currentDate);
    // Serial.print(", LightResistor:");
    // Serial.print(lightReading);
    // Serial.print(", MotionDetector:");
    // Serial.print(motionReading);
    // Serial.print(", is it day?:");
    // Serial.print(IsItDay());
    // Serial.print(", IsLightOn?:");
    // Serial.print(IsLightOn);
    // Serial.print(", MotionTimerStarted?:");
    // Serial.print(MotionTimerStarted);
    // Serial.print(", autoLightOn?:");
    // Serial.print(autoLightOn);
    // Serial.println();
  }
  delay(50);

  Blynk.run();
}

    Downloads, docs, tutorials: https://www.blynk.io
    Sketch generator:           https://examples.blynk.cc
    Blynk community:            https://community.blynk.cc
    Follow us:                  https://www.fb.com/blynkapp
                                https://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on ESP32 chip.

  NOTE: This requires ESP32 support package:
    https://github.com/espressif/arduino-esp32

  Please be sure to select the right ESP32 module
  in the Tools -> Board menu!

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */
// #define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL6TCXFG3W3"
#define BLYNK_TEMPLATE_NAME "Switch"
#define BLYNK_AUTH_TOKEN "Ia6NTh9JH3kUyqYA2zZwyv1gWAIpbK3t"

#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <time.h>

WidgetRTC rtc;


// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Home";
char pass[] = "Mita63249484";

#define servoPin 19
#define HC_SR501 13
#define LightResistor 32

const int LightBorder = 1400;
bool IsLightOn = true;
time_t LightTimerStartTime;
const int LightRegisterTime = 3;  //in seconds and not millis!!
bool LightTimerStarted = false;

time_t MotionTimerStartTime;
const int MotionRegisterTime = 1200;   //twenty minutes
// const int MotionRegisterTime = 10;  //for expiriment, 10 sec
bool MotionTimerStarted = false;

time_t autoSwitchActionStartTime;
const int autoSwitchActionValidTimeWidth = 5; //if within 5 secs after the light turns on the user turns the light back off, it means that light should be kept off.
int lightIsReallyOffValidationCounter = 0;
const int lightIsReallyOffValidationCount = 7;    //to make sure that for seven frames, the light registered off!
bool autoLightOn = true;
bool afterLightOn = false;

int dayStartHour = 8;
int dayEndHour = 23;


time_t now_;
bool motionReading;
int lightReading;

bool IsItDay() {
  if (dayStartHour < dayEndHour) {
    return (dayStartHour < hour() && hour() < dayEndHour);
  } else {
    return (!(dayEndHour < hour() && hour() < dayStartHour));
  }
}

BLYNK_CONNECTED() {
  // Synchronize time on connection
  rtc.begin();
}

Servo myServo;
BLYNK_WRITE(V0) {
  int pinValue = param.asInt();  // assigning incoming value from pin V0 to a variable
  if (pinValue == 1) {
    turnOnLight();
  } else {
    turnOffLight();
  }
}
void turnOnLight() {
  myServo.attach(servoPin);
  myServo.write(63);  // turn servo to 63 degree if switch is ON
  delay(300);
  myServo.write(35);
  myServo.detach();
}
void turnOffLight() {
  myServo.attach(servoPin);
  myServo.write(8);  // turn servo to 8 degree if switch is OFF
  delay(300);
  myServo.write(35);
  myServo.detach();
}

void setup() {
  // Serial.begin(9600);
  pinMode(HC_SR501, INPUT);
  myServo.attach(servoPin);
  myServo.write(35);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  // setSyncInterval(60 * 60 * 24);  // Sync interval in seconds (every day)
  delay(700);
  myServo.detach();
}
void loop() {

  now_ = now();
  motionReading = digitalRead(HC_SR501);
  lightReading = analogRead(LightResistor);
  if (now_ > 1714405458) {

    if (!IsLightOn == (lightReading > LightBorder)) {
      if (LightTimerStarted == false) {
        LightTimerStartTime = now_;
        LightTimerStarted = true;
      } else if (now_ - LightTimerStartTime > LightRegisterTime) {
        IsLightOn = !IsLightOn;
        LightTimerStarted = false;
        autoSwitchActionStartTime = now_;
      }
    } else {
      LightTimerStarted = false;
    }




    if (autoLightOn) {
      //if its night, only automatically turn off and if day, turn off and on automatically.
      if (IsItDay() && !IsLightOn && !LightTimerStarted && motionReading) {
        IsLightOn = true;
        turnOnLight();
        autoSwitchActionStartTime = now_;
        lightIsReallyOffValidationCounter = 0;
        afterLightOn = true;
      } else if (IsLightOn && !LightTimerStarted && !motionReading) {
        if (MotionTimerStarted == false) {
          MotionTimerStartTime = now_;
          MotionTimerStarted = true;
        } else if (now_ - MotionTimerStartTime >= MotionRegisterTime) {
          turnOffLight();
          // Serial.println("lightoff!");
          IsLightOn = false;
          MotionTimerStarted = false;
        }
      }
    }
    if (motionReading) {
      MotionTimerStarted = false;
    }




    if (afterLightOn) {
      if (now_ - autoSwitchActionStartTime < autoSwitchActionValidTimeWidth && lightReading < LightBorder) {    //analogRead(LightResistor) < LightBorder means that its dark in the room
        if (lightIsReallyOffValidationCounter < lightIsReallyOffValidationCount) {
          lightIsReallyOffValidationCounter++;
          // Serial.println(lightIsReallyOffValidationCounter);
        } else {
          autoLightOn = false;
          afterLightOn = false;
          lightIsReallyOffValidationCounter = 0;
          // Serial.println("ligh off disabled!");
        }
      } else if (now_ - autoSwitchActionStartTime > autoSwitchActionValidTimeWidth) {
        afterLightOn = false;
        lightIsReallyOffValidationCounter = 0;
      }
    }
    if (lightReading > LightBorder) {
      autoLightOn = true;
      afterLightOn = false;
      lightIsReallyOffValidationCounter = 0;
      // Serial.println("light on enabled!");
    }
    // String currentTime = String(hour()) + ":" + minute() + ":" + second();
    // String currentDate = String(day()) + " " + month() + " " + year();
    // Serial.print("Current time: ");
    // Serial.print(currentTime);
    // Serial.print(" ");
    // Serial.print(currentDate);
    // Serial.print(", LightResistor:");
    // Serial.print(lightReading);
    // Serial.print(", MotionDetector:");
    // Serial.print(motionReading);
    // Serial.print(", is it day?:");
    // Serial.print(IsItDay());
    // Serial.print(", IsLightOn?:");
    // Serial.print(IsLightOn);
    // Serial.print(", MotionTimerStarted?:");
    // Serial.print(MotionTimerStarted);
    // Serial.print(", autoLightOn?:");
    // Serial.print(autoLightOn);
    // Serial.println();
  }
  delay(50);

  Blynk.run();
}
