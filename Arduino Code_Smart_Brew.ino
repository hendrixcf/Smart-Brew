///////////////////////////////////////////////////////////////////////////

// Project name: Smart-Brew
// Date: 4/25/2022
// Authors: Ricardo Brambila, Collin Hendrix, James McKinley
// Description: Arduino code for coffee maker

///////////////////////////////////////////////////////////////////////////

#include <SoftwareSerial.h> // for bluetooth communication
SoftwareSerial mySerial(2, 3); // for bluetooth communication

#include <OneWire.h> //Temperature sensor
#include <DallasTemperature.h> // Temp Sensor

// Data wire is plugged into digital pin 4 on the Arduino
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"

RTC_DS3231 rtc;

/////// GPIO Set Up//////////////////////////////////////////////////////////
int relay_top_valve_close = 13;  //relay 8
int relay_top_valve_open = 12; //relay 7
int relay_bottom_valve_close = 11;  //relay 6
int relay_bottom_valve_open = 10; //relay 5
int relay_duct_open = 9; // relay 4
int relay_duct_close = 8; //relay 3
int dc_motor = 5; // relay 1
int heating_element = 6; //relay 2

///////////////////////////////////////////////////////////////////////////

// Data recieved by Android phone
int water_temperature;
unsigned long grind_time;
unsigned long steep_time_minutes;
unsigned long steep_time_seconds;
int MONTH;
int DAY;
int YEAR;
int HOUR;
int MINUTE;

int water_temperature_current; // Current celcuis temperature read by sensor

unsigned long valve_delay = 4500; // Valve takes roughly 4-5 seconds to open/close
unsigned long actuator_delay = 6000; // Linear actuator takes roughly 6 seconds to pull/push

bool brewing_in_progress = false;
bool motor_on;

char notification = '1';

void setup(void)
{
  
  pinMode(relay_top_valve_open, OUTPUT); // Opens top valve
  pinMode(relay_top_valve_close, OUTPUT); // Closes top valve
  pinMode(relay_bottom_valve_open, OUTPUT); // Opens bottom valve
  pinMode(relay_bottom_valve_close, OUTPUT); // Closes bottom valve
  pinMode(relay_duct_open, OUTPUT); // Opens duct
  pinMode(relay_duct_close, OUTPUT); // Closes duct
  pinMode(dc_motor, OUTPUT); // Turns dc motor on/off
  pinMode(heating_element, OUTPUT); //Turns heating element on/of
  
  sensors.begin();  // Start up the library
  mySerial.begin(9600);
  Serial.begin(9600);

  //////////////////////////////////RTC SET UP//////////////////////////////

#ifndef ESP8266
  while (!Serial); // Wait for serial port to connect. Needed for native USB
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

}
////////////////////////////////////////////////////////////////////////////////

void  open_bottom() {
  digitalWrite(relay_bottom_valve_open, HIGH);
  delay(valve_delay);
  digitalWrite(relay_bottom_valve_open, LOW);
}

void  close_bottom() {
  digitalWrite(relay_bottom_valve_close, HIGH);
  delay(valve_delay);
  digitalWrite(relay_bottom_valve_close, LOW);
}

void  open_top() {
  digitalWrite(relay_top_valve_open, HIGH);
  delay(valve_delay);
  digitalWrite(relay_top_valve_open, LOW);
}

void  close_top() {
  digitalWrite(relay_top_valve_close, HIGH);
  delay(valve_delay);
  digitalWrite(relay_top_valve_close, LOW);
}

void  open_duct() {
  digitalWrite(relay_duct_open, HIGH);
  delay(actuator_delay);
  digitalWrite(relay_duct_open, LOW);
}

void  close_duct() {
  digitalWrite(relay_duct_close, HIGH);
  delay(actuator_delay);
  digitalWrite(relay_duct_close, LOW);
}

void dc_motor_on() {
  digitalWrite(dc_motor, HIGH);
}
void dc_motor_off() {
  digitalWrite(dc_motor, LOW);
}
void heating_element_on() {
  digitalWrite(heating_element, HIGH);
}

void heating_element_off() {
  digitalWrite(heating_element, LOW);
}

int get_temperature() {
  // Send the command to get temperatures
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

void loop(void)
{
  
  DateTime now = rtc.now();

  if (mySerial.available()) {
      MONTH = mySerial.read();
    }
    
    if (mySerial.available()) {
      DAY = mySerial.read();
    }
    
    if (mySerial.available()) {
      YEAR = mySerial.read();
    }
    
    if (mySerial.available()) {
      HOUR = mySerial.read();
    }

    if (mySerial.available()) {
      MINUTE = mySerial.read();
    }
    
    if (mySerial.available()) {
      water_temperature = mySerial.read();
    }
    
    if (mySerial.available()) {
      grind_time = mySerial.read();
    }
    
    if (mySerial.available()) {
      steep_time_minutes = mySerial.read();
    }
    
    if (mySerial.available()) {
      steep_time_seconds = mySerial.read();
    }
    delay(1000);

    if ((MONTH == now.month()) && (DAY == now.day()) && (HOUR == now.hour()) && (MINUTE == now.minute())){
    
      dc_motor_on();
      heating_element_on();

      unsigned long previous_millis = millis();
      unsigned long current_millis;
      grind_time = grind_time * 1000; // Convert s to ms
      steep_time_minutes = steep_time_minutes * 60000; // Convert min to ms
      steep_time_seconds = steep_time_seconds * 1000; // Convert s to ms
      unsigned long steep_time = steep_time_minutes + steep_time_seconds;
      bool motor_on = true;

      while (true)
      {
        water_temperature_current = get_temperature();
        current_millis = millis();
        if ((unsigned long)(current_millis - previous_millis) >= grind_time && motor_on) // millis() returns number of milliseonds passed since Arduino started running program
        {
          dc_motor_off();
          close_duct();
          motor_on = false;
        }
        water_temperature_current = get_temperature();
        if (water_temperature_current >= water_temperature)
        {
          heating_element_off();
          delay(20000); // Heating element needs to be turned off for at least 15 seconds before being removed from water
          open_top();
          delay(10000); // Time for water to flow from boiling chamber to brewing chamber
          brewing_in_progress = true;
          break;
        }
      }
      
      if(brewing_in_progress = true) 
	    {
        delay(steep_time);
        open_bottom(); 
        delay(30000); // Delay for 30 seconds to drain coffee into cup
        char notification = '1';
        mySerial.write(notification); // Notify user that coffee is ready
  
        // Device settings for next brew
        close_top();
        delay(1000);
        close_bottom();
        delay(1000);
        open_duct();
      } 
    }
  }
