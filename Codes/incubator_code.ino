#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "RTClib.h" //RTC
#include "RF24.h" //NRF 
#include <EEPROM.h>
#include "DHT.h"
#include <Servo.h>
#include <AT24CX.h>
uint8_t address[][6] = {"1Node", "2Node"};
bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit
Servo servou;  // create servo object to control a servo
Servo servod;  // create servo object to control a servo
Servo tray;  // create servo object to control a servo
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

RF24 radio(7, 8); // using pin 7 for the CE pin, and pin 8 for the CSN pin

DHT dhtu(3, DHT22);
DHT dhtd(4, DHT22);
RTC_DS3231 rtc;
AT24CX mem;

//servo 1, 2 and 3 = 3, 4 and 6
const int door = A0;
const int heater = 2;
const int fan = 9;
const int humidity = 5;
const int buttons = A3;
const int buzzer = 10;
const int upv = 1010;
const int downv = 390;
const int enterv = 320;
const int backv = 220;

struct readval {
  float hu;
  float tu;
  float hd;
  float td;
  float oldtd;
  float oldhd;
  byte dd;
  byte mm;
  byte incday;
  boolean vent;
  boolean fan;
  boolean door;
  boolean online;
  boolean wifi;
  byte storehh;
  byte storemm;
  byte cool;
} readings;

struct transfering {
  byte reply;
  double hu;
  double hd;
  double tu;
  double td;
  byte vent;
  byte rot;
  byte dd;
  byte hh;
  byte mn;
  byte mm;
  int yy;
  byte controller;
  byte server;
  byte command;
} mydatas, mydatar;

struct Data {
  double th;
  double tl;
  double hh;
  double hl;
  byte rot;
  byte vent;
  byte dd;
} storedata;

struct Datas {
  byte runing;
  byte startdd;
  byte startmn;
  byte startyy;
  byte startthh;
  byte startmm;
  byte totaldays;
  bool incubating;
} gsettings;

struct timesi {
  byte hh;
  byte mm;
  byte ss;
  byte dd;
  byte mn;
  int yy;
} thetime;

struct makeit {
  bool door;
  bool sensor1;
  bool sensor2;
  bool nrf;
  bool rtc;
} error;
unsigned long onlinetime = 0, starttray = 0, startvent = 0, starttime = 0;
unsigned long errortime = 0, transporttime, startpending = 0, thg = 0, startcommand = 0;
int i, displayPos = 1, count = 0;
byte tempbyte, zone = 0, infortype = 1, oldtraypos, pendingn, tempos = 0;
String information;
bool pressed, transport = false, alarm = false, buzzerstate = false;
bool pending, canregulate = true, commanding = false, ventilation = false;

void setup() {
  Serial.begin(9600);
  servou.write(180);
  servou.attach(A1);  // attaches the servo on pin 9 to the servo object
  servou.write(180);
  servou.detach();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("  TROPICALISED");
  lcd.setCursor(2, 1);
  lcd.print("EGG INCUBATOR");
  pinMode(door, INPUT);
  pinMode(heater, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(humidity, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  digitalWrite(fan, LOW);
  readings.fan = false;
  digitalWrite(humidity, LOW);
  digitalWrite(heater, LOW);
  if (!rtc.begin()) error.rtc = true;
  else error.rtc = false;
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  dhtu.begin();
  dhtd.begin();
  readings.vent = false;
  readings.cool = false;
  initialized();
  starttray = millis();
  configure();
}
void loop() {
  if ((commanding == true) && (millis() - startcommand >= 30000)) {
    commanding = false;
    digitalWrite(fan, LOW);
    readings.fan = false;
    digitalWrite(buzzer, LOW);
    if (tempos != oldtraypos) movetray(tempos);
    if (ventilation != readings.vent) {
      if (ventilation) {
        readings.vent = ventilation;
        openport();
      }
      else {
        readings.vent = ventilation;
        closeport();
      }
    }
  }
  if (millis() - thg >= 60000) {
    configure();
    thg = millis();
  }
  if ((error.rtc) || (error.sensor1) || (error.sensor2) || (error.door)) {
    if (buzzerstate == false) {
      if (millis() - errortime >= 5000) {
        buzzerstate = true;
        digitalWrite(buzzer, HIGH);
        errortime = millis();
      }
    } else {
      if (millis() - errortime >= 500) {
        buzzerstate = false;
        digitalWrite(buzzer, LOW);
        errortime = millis();
      }
    }
  }
  if (radio.available() > 0) {
    thg = millis();
    radio.read(&mydatar, sizeof(mydatar));
    readings.online = true;
    onlinetime = millis();
    process();
  }
  if (millis() - onlinetime >= 30000) {
    readings.online = false;
  }
  if (millis() - starttime >= 2000) {
    readvar();
    readtime();
    if (readings.tu == 0) error.sensor1 = true;
    else error.sensor1 = false;
    if (readings.td == 0) error.sensor2 = true;
    else error.sensor2 = false;
    if ((thetime.mm == 30) || (thetime.mm == 0)) {
      if ((readings.storehh != thetime.hh) || (readings.storemm != thetime.mm)) {
        readings.storehh = thetime.hh;
        readings.storemm = thetime.mm;
        writeEE(readings.incday, readings.storehh, readings.storemm);
      }
      if (readings.dd != thetime.dd) {
        loadvalue();
        readings.dd = thetime.dd;
      }
    }
    if ((transport == false) && (canregulate == true)) {
      regulate();
    }
    if (digitalRead(A0) == HIGH) {
      error.door = true;
      if (gsettings.incubating) {
        regulate();
      }
    } else error.door = false;
    if (transport == true) {
      if (millis() - transporttime >= 15000) transport = false;
      transport = false;
    } else {
      count++;
      if (count >= 2) {
        count = 0;
        if (displayPos + 1 <= 4) displayPos++;
        else displayPos = 1;
        updateDisplay();
      }
    }
    starttime = millis();
  }
}
void configure() {
  if (!radio.begin()) {
    error.nrf = true;
    lcd.clear();
    lcd.print("NO COMMUNICATION");
    lcd.setCursor(0, 1);
    lcd.print("CHECK MODULE");
    delay(1000);
  } else error.nrf = false;
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(mydatar));
  radio.openWritingPipe(address[radioNumber]);
  radio.openReadingPipe(1, address[!radioNumber]);
  radio.startListening();
}
void buttonp() {
  digitalWrite(buzzer, HIGH);
  delay(100);
  digitalWrite(buzzer, 0);
}
void loadvalue() {
  readtime();
  byte cdd;
  if (gsettings.startmn == thetime.mn) {
    cdd = (thetime.dd - gsettings.startdd) + 1;
  } else {
    if ((thetime.mn - 1 == 9) || (thetime.mn - 1 == 4) || (thetime.mn - 1 == 11))
      cdd = (30 - gsettings.startdd) + thetime.dd;
    else if (thetime.mn - 1 == 2)
      cdd = ((29 - gsettings.startdd) + thetime.dd) - (thetime.yy % 4);
    else cdd = (31 - gsettings.startdd) + thetime.dd;
  }
  if (error.rtc) cdd = 10;
  readings.incday = cdd;
  EEPROM.get(cdd * 20, storedata);
}
void regulate() {
  if (commanding) {
    digitalWrite(heater, LOW);
    digitalWrite(humidity, LOW);
    return;
  }
  if (gsettings.incubating == true) {
    if ((error.sensor1) && (error.sensor2)) {
      digitalWrite(fan, LOW);
      digitalWrite(heater, LOW);
      digitalWrite(humidity, LOW);
      return;
    }
    bool portcontrol = true;
    double tempdiff = abs(readings.tu - readings.td);
    double humiddiff = abs(readings.hu - readings.hd);

    if (error.door == true) {
      readings.fan = false;
      digitalWrite(fan, LOW);
      if (readings.cool == true) {
        closeport();
        readings.cool = false;
      }
      if (readings.td < storedata.tl) {
        digitalWrite(heater, HIGH);
        digitalWrite(humidity, LOW);
        if (readings.vent == true) {
          closeport();
          portcontrol = false;
          readings.vent = false;
        }
      } else {
        if (readings.hd < storedata.hh) {
          digitalWrite(humidity, HIGH);
        }
      }
    } else {
      if ((readings.td >= storedata.th) || (readings.tu >= storedata.th)) {
        zone = 3;
        digitalWrite(fan, HIGH);
        digitalWrite(humidity, LOW);
        digitalWrite(heater, LOW);
        if (error.door == false) {
          portcontrol = false;
          if (readings.cool == false) {
            readings.cool = true;
            openport();
            startvent = millis();
          }
        }
      }
      else if (readings.td < (((storedata.th + storedata.tl) / 2.0)))   {
        zone = 2;
        digitalWrite(fan, HIGH);
        digitalWrite(heater, HIGH);
        readings.fan = true;
        if (readings.cool == true) {
          closeport();
          portcontrol = false;
          readings.cool = false;
        }
      }
      else if (readings.td < storedata.th)   {
        zone = 1;
        digitalWrite(heater, LOW);
        if (readings.cool == true) {
          closeport();
          readings.cool = false;
        }
        if (readings.hd < storedata.hl) {
          digitalWrite(humidity, HIGH);
        } else if (readings.hd > storedata.hh) {
          digitalWrite(humidity, LOW);
        }
        if ((tempdiff > 0.5) || (humiddiff >= 5)) {
          digitalWrite(fan, HIGH);
          readings.fan = true;
        } else {
          digitalWrite(fan, LOW);
          readings.fan = false;
        }
      }
      
      if (storedata.rot > 0) {
        if (((millis() - starttray) / 1000.0) >= (86400 / storedata.rot)) {
          if (oldtraypos + 1 <= 2) movetray(oldtraypos + 1);
          else movetray(0);
          starttray = millis();
        }
      } else {
        if (oldtraypos != 1) {
          movetray(1);
        }
      }
      
      if (storedata.vent > 0) {
        if (readings.vent == true) {
          if (millis() - startvent >= 60000) {
            closeport();
            readings.vent = false;
            startvent = millis();
          }
        } else {
          if (((millis() - startvent) / 1000.0) >= (86400 / storedata.vent)) {
            openport();
            readings.vent = true;
            startvent = millis();
          }
        }
      } else if (storedata.vent == 0) {
        if (readings.vent == true) {
          closeport();
          readings.vent = false;
        }
      }
    }
    
  }
  
  else {
    digitalWrite(heater, LOW);
    digitalWrite(humidity, LOW);
    digitalWrite(fan, LOW);
  }
}

void process() {
  if ((mydatar.reply == 0) && (mydatar.controller == 13) && (mydatar.server == 14)) {
    mydatas.reply = 1;
    mydatas.controller = 13;
    mydatas.server = 14;
    mydatas.command = mydatar.command;
    switch (mydatar.command) {
      case 0: {
          digitalWrite(buzzer, HIGH);
          delay(100);
          digitalWrite(buzzer, LOW);
          gsettings.incubating = false;
          EEPROM.put(1000, gsettings);
          transmit();
          break;
        }
      case 1: { //startincubating;
          lcd.clear();
          lcd.print(" CLEANING RECORD");
          lcd.setCursor(0, 1);
          lcd.print("MEMORY........");
          for (i = 0; i <= (24 * 348); i += 4) mem.writeFloat(i, 0);
          gsettings.startdd = mydatar.dd;
          gsettings.startmn = mydatar.mn;
          gsettings.startyy = mydatar.yy;
          gsettings.startthh = mydatar.hh;
          gsettings.startmm = mydatar.mm;
          gsettings.incubating = true;
          readings.dd = 1;
          EEPROM.put(1000, gsettings);
          loadvalue();
          transmit();
          break;
        }
      case 2: { //receive data for day n
          lcd.clear();
          lcd.print("REVEIVING DATA");
          lcd.setCursor(2, 1);
          lcd.print("FOR DAY "); lcd.print(mydatar.dd);
          startpending = millis();
          pendingn = mydatar.dd;
          pending = true;
          storedata.hh = mydatar.hu;
          storedata.hl = mydatar.hd;
          storedata.th = mydatar.tu;
          storedata.tl = mydatar.td;
          storedata.vent = mydatar.vent;
          storedata.rot = mydatar.rot;
          storedata.dd = mydatar.dd;
          EEPROM.put(mydatar.dd * 20, storedata);
          loadvalue();
          break;
        }
      case 3: { //correct date and time
          thetime.hh = mydatar.hh;
          thetime.mm = mydatar.mm;
          thetime.dd = mydatar.dd;
          thetime.yy = mydatar.yy;
          thetime.mn = mydatar.mn;
          thetime.ss = 0;
          rtc.adjust(DateTime(thetime.yy, thetime.mn, thetime.dd, thetime.hh, thetime.mm, thetime.ss));
          readtime();
          transmit();
          break;
        }
      case 4: { //return date and time
          readtime();
          transmit();
          break;
        }
      case 5: {
          transfered(mydatar.dd);
          break;
        }
      case 6: {
          if ((pending == true) && (millis() - startpending >= 10000)) {
            pending = false;
            mydatas.command = 2;
            mydatas.dd = pendingn;
            transmit();
          } else {
            mydatas.hu = readings.hu;
            mydatas.hd = readings.hd;
            mydatas.tu = readings.tu;
            mydatas.td = readings.td;
            transmit();
          }
          break;
        }
      case 7: {
          startcommand = millis();
          if (commanding == false) {
            tempos = oldtraypos;
            ventilation = readings.vent;
            commanding = true;
          }
          switch (mydatar.yy) {
            case 1: {
                byte pos = oldtraypos;
                if (pos + 1 <= 4) pos++;
                else pos = 0;
                movetray(pos);
                mydatas.vent = pos;
                transmit();
                break;
              }
            case 2: {
                if (mydatar.vent == 1)  {
                  digitalWrite(fan, HIGH);
                  readings.fan = true;
                  mydatas.vent = 1;
                  transmit();
                } else {
                  digitalWrite(fan, LOW);
                  readings.fan = false;
                  mydatas.vent = 0;
                  transmit();
                }
                break;
              }
            case 3: {
                if (mydatar.vent == 1)  {
                  openport();
                  mydatas.vent = 1;
                  transmit();
                } else {
                  closeport();
                  mydatas.vent = 0;
                  transmit();
                }
                break;
              }
          }
          break;
        }
    }
  }
}

void menu() {
  byte tempos = oldtraypos;
  bool ventilation = readings.vent;
  byte select = 1;
  int val;
  i = 1;
  bool pressed = true;
  while (1) {
    if (millis() - starttime >= 15000) {
      lcd.clear();
      lcd.print("TAKING TOO LONG");
      lcd.setCursor(0, 1);
      lcd.print("RETURNING......");
      starttime = millis();
      break;
    }

    val = analogRead(buttons);
    if (val >= upv) {
      switch (select) {
        case 1: {
            byte pos = oldtraypos;
            if (pos + 1 <= 4) pos++;
            else pos = 0;
            movetray(pos);
            break;
          }
        case 2: {
            digitalWrite(fan, HIGH);
            readings.fan = true;
            break;
          }
        case 3: {
            openport();
            break;
          }
      }
      pressed = true;
    } else if (val >= downv) {
      switch (select) {
        case 1: {
            byte pos = oldtraypos;
            if (pos + 1 <= 4) pos++;
            else pos = 0;
            movetray(pos);
            break;
          }
        case 2: {
            digitalWrite(fan, LOW);
            readings.fan = false;
            break;
          }
        case 3: {
            closeport();
            break;
          }
      }
      pressed = true;
    } else if (val >= enterv) {
      if (select + 1 <= 3) select++;
      else select = 1;
      pressed = true;
    }
    else if (val >= backv) {
      updateDisplay();
      break;
    }
    if (pressed == true) {
      starttime = millis();
      //Serial.println(val);
      buttonp();
      pressed = false;
      starttime = millis();
      lcd.clear();
      switch (select) {
        case 1: {
            lcd.print("TRAY ORIENTATION");
            lcd.setCursor(0, 1);
            lcd.print("CURRENT: "); lcd.print(oldtraypos);
            break;
          }
        case 2: {
            lcd.print("FANS STATE");
            lcd.setCursor(0, 1);
            lcd.print("CURRENT: ");
            if (readings.fan) lcd.print("ONNED");
            else lcd.print("OFFED");
            break;
          }
        case 3: {
            lcd.print("PORTS STATE");
            lcd.setCursor(0, 1);
            lcd.print("CURRENT: ");
            if (readings.vent) lcd.print("OPENED");
            else lcd.print("CLOSED");
            break;
          }
      }
      delay(200);
      while (analogRead(buttons) >= backv);
    }
  }

  digitalWrite(fan, LOW);
  readings.fan = false;
  digitalWrite(buzzer, LOW);
  if (tempos != oldtraypos) movetray(tempos);
  if (ventilation != readings.vent) {
    if (ventilation) {
      readings.vent = ventilation;
      openport();
    }
    else {
      readings.vent = ventilation;
      closeport();
    }
  }
}
void transporting() {
  lcd.clear();
  lcd.print("TRANSPORTATION...");
  movetray(2);
  closeport();
  digitalWrite(fan, LOW);
  digitalWrite(humidity, 0);
  digitalWrite(fan, LOW);
  digitalWrite(buzzer, HIGH);
  delay(700);
  digitalWrite(buzzer, LOW);
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("READY FOR ");
  lcd.setCursor(0, 1);
  lcd.print("TRANSPORTATION");
  transport = true;
  transporttime = millis();
}
void closeport() {
  servou.attach(A1);
  digitalWrite(buzzer, LOW);
  for (i = 0; i < 180; i++) {
    servou.write(i);
    delay(30);
  }
  servou.detach();
}
void openport() {
  servou.attach(A1);  // attaches the servo on pin 9 to the servo object
  digitalWrite(buzzer, LOW);
  for (i = 180; i >= 0; i--) {
    servou.write(i);
    delay(30);
  }
  servou.detach();
}
void movetray(int traypos) {
  tray.attach(6);
  digitalWrite(buzzer, LOW);
  EEPROM.write(0, traypos);
  byte oldpos, newpos, ttraypos;
  while (oldtraypos != traypos) {
    ttraypos = oldtraypos;
    if (oldtraypos + 1 <= 4) oldtraypos++;
    else oldtraypos = 0;
    oldpos = 45 + 30 * ttraypos;
    newpos = 45 + 30 * oldtraypos;
    if (oldpos < newpos) {
      for (i = oldpos; i <= newpos; i++) {
        tray.write(i);
        delay(100);
      }
    } else {
      for (i = oldpos; i >= newpos; i--) {
        tray.write(i);
        delay(100);
      }
    }
  }
  oldtraypos = traypos;
  tray.detach();
}
void initialized() {
  EEPROM.get(1000, gsettings);
  if (gsettings.runing == 1) {
    oldtraypos = EEPROM.read(0);
  } else {
    gsettings.runing = 1;
    gsettings.startdd = 23;
    gsettings.startmn = 6;
    gsettings.startyy = 2022;
    gsettings.startthh = 12;
    gsettings.startmm = 40;
    gsettings.totaldays = 21;
    gsettings.incubating = false;
    oldtraypos = 2;
    EEPROM.write(0, oldtraypos);
    EEPROM.put(1000, gsettings);
  }
  readvar();
  readings.dd = thetime.dd;
  loadvalue();
  regulate();
  if (gsettings.incubating == false) {
    digitalWrite(heater, LOW);
    digitalWrite(humidity, LOW);
    digitalWrite(fan, LOW);
  }
  if (storedata.rot > 0) {
    if (oldtraypos + 1 <= 4) movetray(oldtraypos + 1);
    else movetray(0);
  }
}
void transmit() {
  mydatas.controller = 13;
  mydatas.server = 14;
  radio.stopListening();
  radio.write(&mydatas, sizeof(mydatas));
  radio.startListening();
}

void readvar() {
  readings.hu = dhtu.readHumidity();
  readings.tu = dhtu.readTemperature();
  if (isnan(readings.hu) || isnan(readings.tu)) {
    readings.hu = 0;
    readings.tu = 0;
  }
  readings.hd = dhtd.readHumidity();
  readings.td = dhtd.readTemperature();
  if (isnan(readings.hd) || isnan(readings.td)) {
    readings.hd = 0;
    readings.td = 0;
  }
  if ((readings.hd == 0) && (readings.hu != 0)) {
    readings.hd = readings.hu;
    readings.td = readings.tu;
  }
  if ((readings.hu == 0) && (readings.hd != 0)) {
    readings.hu = readings.hd;
    readings.tu = readings.td;
  }
}
void transfered(int theday) {
  for (int k = 0; k <= 23; k++) {
    readEE(theday, k, 0);
    delay(100);
    readEE(theday, k, 30);
    delay(100);
  }
}
void writeEE(byte dd, byte hh, byte mm) {
  double t =  max(readings.td, readings.tu);
  double h = max(readings.hd, readings.hu);
  int divider = int(mm / 30);
  int location = (dd * 348) + (hh * 16) + divider * 8;
  mem.writeFloat(location, t);
  mem.writeFloat(location + 4, h);
}
void readEE(byte d, byte h, byte m) {
  mydatas.controller = 13;
  mydatas.server = 14;
  mydatas.mm = m;
  mydatas.dd = d;
  mydatas.hh = h;
  int divider = int(m / 30);
  int location = (d * 348) + (h * 16) + divider * 8;
  double tt = mem.readFloat(location);
  double hh = mem.readFloat(location + 4);
  mydatas.td = tt;
  mydatas.hd = hh;
  mydatas.reply = 0;
  mydatas.command = 5;
  transmit();
}
void updateDisplay() { //UPDATE THE DISPLAY TO REFLECT ANY NEW CHANGE
  lcd.clear();
  switch (displayPos) {
    case 1: {
        if (!error.sensor1) {
          if (zone == 1) lcd.print("M");
          else if (zone == 2) lcd.print("H");
          else if (zone == 3) lcd.print("C");
          else lcd.print("U");
          lcd.setCursor(2, 0);
          lcd.print(readings.tu, 1); lcd.print(char(223)); lcd.print("C  ");
          lcd.print(readings.hu, 1); lcd.print("%");
        } else {
          lcd.print(F("  ERROR CODE 2"));
        }
        if (!error.sensor2) {
          lcd.setCursor(2, 1);
          lcd.print(readings.td, 1); lcd.print(char(223)); lcd.print("C  ");
          lcd.print(readings.hd, 1); lcd.print("%");
        } else {
          lcd.setCursor(0, 1);
          lcd.print(F("  ERROR CODE 3"));
        }
        break;
      }
    case 2: {
        if (!error.rtc) {
          readtime();
          lcd.print(F("DATE: ")); lcd.print(thetime.dd); lcd.print("/");
          lcd.print(thetime.mn); lcd.print("/"); lcd.print(thetime.yy);
          lcd.setCursor(0, 1);
          lcd.print(F("TIME: ")); lcd.print(thetime.hh); lcd.print(":"); lcd.print(thetime.mm);
          lcd.print(":"); lcd.print(thetime.ss);
        } else {
          lcd.print(F("  ERROR CODE 4"));
          lcd.setCursor(0, 1);
          lcd.print("(REAL TIME CLOCK)");
        }
        break;
      }
    case 3: {
        if (gsettings.incubating) {
          lcd.print(F(" INCUBATION DAY"));
          lcd.setCursor(5, 1);
          lcd.print(readings.incday);
          lcd.print(" OF ");
          lcd.print(gsettings.totaldays);
        } else {
          lcd.setCursor(1, 0);
          lcd.print("NOT INCUBATING");
          lcd.setCursor(2, 1);
          lcd.print("STANDBY MODE");
        }
        break;
      }
    case 4: {
        if (error.door) {
          lcd.print(F("  ERROR CODE 5"));
          lcd.setCursor(2, 1);
          lcd.print("(DOOR OPENED)");
        } else if (error.nrf) {
          lcd.print(F("  ERROR CODE 1"));
          lcd.setCursor(0, 1);
          lcd.print("(COMMUNICATION)");
        } else {
          lcd.print("T ");
          lcd.print(storedata.th, 1); lcd.print(char(223)); lcd.print("C  ");
          lcd.print(storedata.hh, 1); lcd.print("%");
          lcd.setCursor(0, 1);
          lcd.print("T ");
          lcd.print(storedata.tl, 1); lcd.print(char(223)); lcd.print("C  ");
          lcd.print(storedata.hl, 1); lcd.print("%");
        }
        break;
      }
  }
}
void readtime() {
  DateTime now = rtc.now();
  thetime.hh = now.hour();
  thetime.mm = now.minute();
  thetime.ss = now.second();
  thetime.dd = now.day();
  thetime.mn = now.month();
  thetime.yy = now.year();
  mydatas.hh = thetime.hh;
  mydatas.mm = thetime.mm;
  mydatas.dd = thetime.dd;
  mydatas.mn = thetime.mn;
  mydatas.yy = thetime.yy;
}
