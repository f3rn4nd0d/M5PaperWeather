/*
   Copyright (C) 2021 SFini

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
   
   Version 2.0: Grajera (Segovia), Open-Meteo data, Spanish interface, Casa Grajera logo.
   Version 1.1 by Fernando Doutel. Modified some parts of the original interface 
   to accomodate Thermal Comfort (AKA feels like temperature outside) plus 
   normalized moon phase
*/
/**
  * @file Weather.h
  * 
  * Main file with setup() and loop()
  */
  
#include <M5EPD.h>
#include "Config.h"
//include "ConfigOverride.h" // Remove this line
#include "Data.h"
#include "Display.h"
#include "Battery.h"
#include "EPD.h"
#include "EPDWifi.h"
#include "Moon.h"
#include "SHT30.h"
#include "Time.h"
#include "Utils.h"
#include "Weather.h"

// Refresh the M5Paper info more often.
// #define REFRESH_PARTLY 1

MyData         myData;            // The collection of the global data
Pages          myDisplay(myData); // The pages (weather x2, Home Assistant) and their drawing

// After every refresh the M5Paper stays awake for a while so the wheel can be used:
//   rotate = previous / next page (redrawn from the data in RAM, no wifi), push = refresh the data now.
#ifndef INTERACT_SECONDS
#define INTERACT_SECONDS 45
#endif

static uint32_t bootMs       = 0;      // millis() when this cycle started
static int      refreshSec   = 60 * 60; // seconds between two refreshes
static uint32_t lastInput    = 0;      // millis() of the last wheel event
static uint32_t pendingSince = 0;      // millis() of the last rotation not yet drawn
static bool     pending      = false;  // a rotation waits for the redraw

/* Wifi + sensors + weather + Home Assistant download. Returns true when the weather data is valid. */
bool UpdateAll()
{
   bool ok = false;

   if (StartWiFi(myData.wifiRSSI)) {
      GetBatteryValues(myData);
      GetSHT30Values(myData);
      for (int attempt = 0; attempt < 3 && !ok; attempt++) {
         ok = myData.weather.Get();
         if (!ok) delay(5000);
      }
      if (ok) {
         SetRTCDateTime(myData);
      }
      GetMoonValues(myData);
      myData.ha.Get();     // read-only, one request; a failure only blanks the Home Assistant page
      myData.Dump();
      myDisplay.Show(myData.page);   // shows a "no data" message when ok is false
      StopWiFi();
   } else {
      GetBatteryValues(myData);
      GetSHT30Values(myData);
      myData.ha.valid = false;
      myDisplay.Show(myData.page);   // no wifi: the weather is not valid, so the no data message is shown
   }
   return ok;
}

/* Read the wheel. Rotation moves the page (drawn after a short pause so fast turns count once),
 * push refreshes the data. Returns true when the data must be refreshed. */
bool ServiceWheel()
{
   bool refresh = false;

   M5.update();
   if (M5.BtnL.wasPressed() || M5.BtnR.wasPressed()) {
      int count = myDisplay.Count();
      int step  = M5.BtnR.wasPressed() ? 1 : count - 1;   // right = next, left = previous

      myData.page   = (myData.page + step) % count;
      pending       = true;
      pendingSince  = millis();
      lastInput     = millis();
   }
   if (M5.BtnP.wasPressed()) {
      refresh   = true;
      lastInput = millis();
   }
   if (pending && millis() - pendingSince > 400) {
      pending = false;
      myData.SaveNVS();
      myDisplay.Show(myData.page);
      lastInput = millis();
   }
   return refresh;
}

/* Start and M5Paper instance */
void setup()
{
   bootMs = millis();
#ifndef REFRESH_PARTLY
   InitEPD(true);
   myData.LoadNVS();
   myData.page = myDisplay.Clamp(myData.page);
   bool ok = UpdateAll();
   refreshSec = ok ? 60 * 60 : RETRY_MINUTES * 60;   // every 1 hour, or retry soon after a failure

   lastInput = millis();
   while (millis() - lastInput < INTERACT_SECONDS * 1000UL) {
      if (ServiceWheel()) {
         ok = UpdateAll();
         refreshSec = ok ? 60 * 60 : RETRY_MINUTES * 60;
         bootMs     = millis();
      }
      delay(10);
   }
   int left = refreshSec - (int) ((millis() - bootMs) / 1000);

   ShutdownEPD(left > 30 ? left : 30);
   // Only reached while the USB cable is connected (the M5Paper cannot power off then):
   // loop() keeps the wheel working and restarts when the refresh is due.
#else
   myData.LoadNVS();
   if (myData.nvsCounter == 1) {
      InitEPD(true);
      UpdateAll();
   } else {
      InitEPD(false);
      GetSHT30Values(myData);
      myDisplay.ShowM5PaperInfo(myData.page);
      if (myData.nvsCounter >= 60) {
         myData.nvsCounter = 0;
      }
   }
   myData.nvsCounter++;
   myData.SaveNVS();
   ShutdownEPD(10); // 1 minute
#endif // REFRESH_PARTLY
}

/* Main loop. Never reached on battery (shutdown); with USB it serves the wheel and restarts when due. */
void loop()
{
#ifndef REFRESH_PARTLY
   if (ServiceWheel()) {
      ESP.restart();   // push = refresh now
   }
   if ((millis() - bootMs) / 1000 >= (uint32_t) refreshSec) {
      ESP.restart();
   }
   delay(10);
#endif
}
