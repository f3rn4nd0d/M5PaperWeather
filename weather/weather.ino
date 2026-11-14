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
WeatherDisplay myDisplay(myData); // The global display helper class

/* Wifi + sensors + weather download. Returns true when the weather data is valid. */
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
      myData.Dump();
      myDisplay.Show();   // shows a "no data" message when ok is false
      StopWiFi();
   } else {
      GetBatteryValues(myData);
      GetSHT30Values(myData);
      myDisplay.Show();   // no wifi: the weather is not valid, so the no data message is shown
   }
   return ok;
}

/* Start and M5Paper instance */
void setup()
{
//Serial.begin(115200); 
#ifndef REFRESH_PARTLY
   InitEPD(true);
   bool ok = UpdateAll();
   ShutdownEPD(ok ? 60 * 60 : RETRY_MINUTES * 60); // every 1 hour, or retry soon after a failure
#else 
   myData.LoadNVS();
   if (myData.nvsCounter == 1) {
      InitEPD(true);
      UpdateAll();
   } else {
      InitEPD(false);
      GetSHT30Values(myData);
      myDisplay.ShowM5PaperInfo();
      if (myData.nvsCounter >= 60) {
         myData.nvsCounter = 0;
      }
   }
   myData.nvsCounter++;
   myData.SaveNVS();
   ShutdownEPD(10); // 1 minute
#endif // REFRESH_PARTLY   
}

/* Main loop. Never reached because of shutdown */
void loop()
{
}
