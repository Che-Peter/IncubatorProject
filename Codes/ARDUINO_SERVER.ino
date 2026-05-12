
#include <SPI.h>
#include "RF24.h"
RF24 radio(9, 10); //CE CSN

uint8_t address[][6] = {"1Node", "2Node"};
bool radioNumber = 0; // homes=0, pole=1;
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
} mydatar, mydatas;

unsigned long starttime = 0, startt, startalarm, startting = 0;
int displaypos, i, j;
String information;
bool authorised = false;
void setup() {
  Serial.begin(9600);
  configure();
  starttime = startt = millis();
  Serial.print("*dready#");
}

void loop() {
  if (millis() - startting >= 50000) {
    configure();
    startting = millis();
  }
  if (Serial.available() > 0) {
    information = Serial.readString();
    delay(5);
    if (information[0] == '6') {
      configure();
      authorised = true;
      mydatas.reply = 0;
      mydatas.controller = 13;
      mydatas.server = 14;
      mydatas.command = 6;
      request();
      starttime = millis();
    }
    else if (information[0] == '7') authorised = false;
    else {
      extractandsend();
    }
    starttime = millis();
  }
  if (radio.available() > 0) {
    radio.read(&mydatar, sizeof(mydatar));
    if ((mydatar.controller == 13) && (mydatar.server == 14)) {
      parcelandsend();
    }
  }
  if (authorised) {
    if (millis() - starttime >= 5000) {
      mydatas.reply = 0;
      mydatas.controller = 13;
      mydatas.server = 14;
      mydatas.command = 6;
      request();
      starttime = millis();
    }
  }
}
void configure() {
  startting = millis();
  radio.begin();
  radio.setPALevel(RF24_PA_MIN);
  radio.setPayloadSize(sizeof(mydatar));
  radio.openWritingPipe(address[radioNumber]);
  radio.openReadingPipe(1, address[!radioNumber]);
  radio.startListening();
}
void parcelandsend() {
  String information = "";
  information = String(mydatar.reply) + String(',') +
                String(mydatar.hu) + String(',') +
                String(mydatar.hd) + String(',') +
                String(mydatar.tu) + String(',') +
                String(mydatar.td) + String(',') +
                String(mydatar.vent) + String(',') +
                String(mydatar.rot) + String(',') +
                String(mydatar.dd) + String(',') +
                String(mydatar.hh) + String(',') +
                String(mydatar.mn) + String(',') +
                String(mydatar.mm) + String(',') +
                String(mydatar.yy) + String(',') +
                String(mydatar.controller) + String(',') +
                String(mydatar.server) + String(',') +
                String(mydatar.command) + String(',');
  information = "*" + information + "#";
  Serial.print(information);
}
void extractandsend() {
  String sub;
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.reply = sub.toInt();
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.hu = sub.toFloat();
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.hd = sub.toFloat();
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.tu = sub.toFloat();
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.td = sub.toFloat();
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.vent = sub.toFloat();
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.rot = sub.toInt();
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.dd = sub.toInt();
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.hh = sub.toInt();
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.mn = sub.toInt();
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.mm = sub.toInt(); //eoveflow on 2
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.yy = sub.toInt(); //eoveflow on 2
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.controller = sub.toInt(); //eoveflow on 2
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.server = sub.toInt(); //eoveflow on 2
  sub = information.substring(0, information.indexOf(','));
  information = information.substring(information.indexOf(',') + 1);
  mydatas.command = sub.toInt(); //eoveflow on 2
  transmit();
}

void request() {
  bool ok = false;
  transmit();
  startt = millis();
  while (millis() - startt <= 2000) {
    if (radio.available()) {
      radio.read(&mydatar, sizeof(mydatar));
      if ((mydatar.controller == 13) && (mydatar.server == 14)) { //send current reading
        parcelandsend();
        ok = true;
        break;
      }
    }
  }
}
void transmit() {
  radio.stopListening();
  radio.write(&mydatas, sizeof(mydatas));      // transmit & save the report
  radio.startListening();
}
