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
*/
/**
  * @file Weather.h
  * 
  * Class for reading all the weather data from openweathermap.
  */
#pragma once
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <math.h>
#include <string.h>
#include "Utils.h"

#define MAX_HOURLY   24
#define MAX_FORECAST  8
#define MIN_RAIN     10

/**
  * Class for reading all the weather data from Open-Meteo (https://open-meteo.com).
  * Open-Meteo needs no API key. Data licence: CC BY 4.0 (attribution required).
  */
class Weather
{
public:
   bool   valid;                           //!< true when the last download was successful
   time_t currentTime;                     //!< Current timestamp (local time)
   int    currentTimeOffset;               //!< Current timezone offset in seconds

   time_t sunrise;                         //!< Sunrise timestamp (local time)
   time_t sunset;                          //!< Sunset timestamp (local time)
   float  winddir;                         //!< Wind direction in degrees
   float  windspeed;                       //!< Wind speed in km/h

   float  currentTemp;                     //!< Current temperature
   float  currentThermalComfort;           //!< Current thermal comfort (feels like)

   time_t hourlyTime[MAX_HOURLY];          //!< timestamp of the hourly forecast (local time)
   float  hourlyMaxTemp[MAX_HOURLY];       //!< temperature forecast
   String hourlyMain[MAX_HOURLY];          //!< short description of the hourly forecast
   String hourlyIcon[MAX_HOURLY];          //!< icon code in the openweathermap style (01d, 10n ...)

   int    maxRain;                         //!< maximum rain in mm of the day forecast
   int    rainProbToday;                   //!< maximum rain probability today in %
   int    tempAxisMin;                     //!< lower bound for the temperature graph
   int    tempAxisMax;                     //!< upper bound for the temperature graph
   int    pressureAxisMin;                 //!< lower bound for the pressure graph
   int    pressureAxisMax;                 //!< upper bound for the pressure graph
   time_t forecastTime[MAX_FORECAST];      //!< local midnight of each forecast day
   float  forecastMaxTemp[MAX_FORECAST];   //!< max temperature
   float  forecastMinTemp[MAX_FORECAST];   //!< min temperature
   float  forecastRain[MAX_FORECAST];      //!< rain sum in mm
   float  forecastHumidity[MAX_FORECAST];  //!< mean humidity of the day
   float  forecastPressure[MAX_FORECAST];  //!< mean air pressure at sea level of the day

protected:
   /* Convert UTC time to local time */
   time_t LocalTime(time_t time)
   {
      return time + currentTimeOffset;
   }

   /* Convert a WMO weather code to the icon names used by Display.h (openweathermap style). */
   static const char *WmoToIcon(int code, bool isDay, char *buf)
   {
      const char *base = "03";

      if      (code == 0)                    base = "01"; // clear sky
      else if (code == 1)                    base = "02"; // mainly clear
      else if (code == 2)                    base = "03"; // partly cloudy
      else if (code == 3)                    base = "04"; // overcast
      else if (code == 45 || code == 48)     base = "50"; // fog
      else if (code >= 51 && code <= 57)     base = "09"; // drizzle
      else if (code >= 61 && code <= 67)     base = "10"; // rain
      else if (code >= 71 && code <= 77)     base = "13"; // snow
      else if (code >= 80 && code <= 82)     base = "09"; // rain showers
      else if (code == 85 || code == 86)     base = "13"; // snow showers
      else if (code >= 95 && code <= 99)     base = "11"; // thunderstorm
      buf[0] = base[0];
      buf[1] = base[1];
      buf[2] = isDay ? 'd' : 'n';
      buf[3] = 0;
      return buf;
   }

   /* Short Spanish description of a WMO weather code (ASCII only). */
   static const char *WmoToText(int code)
   {
      if (code == 0)                    return "Despejado";
      if (code == 1 || code == 2)       return "Poco nuboso";
      if (code == 3)                    return "Nublado";
      if (code == 45 || code == 48)     return "Niebla";
      if (code >= 51 && code <= 57)     return "Llovizna";
      if (code >= 61 && code <= 67)     return "Lluvia";
      if (code >= 71 && code <= 77)     return "Nieve";
      if (code >= 80 && code <= 82)     return "Chubascos";
      if (code == 85 || code == 86)     return "Nevadas";
      if (code >= 95)                   return "Tormenta";
      return "?";
   }

   /* Calls the Open-Meteo request and deserialisation the json data. */
   bool GetOpenMeteoJsonDoc(DynamicJsonDocument &doc)
   {
      // Only the fields we need are kept: this saves a lot of RAM.
      StaticJsonDocument<1024> filter;   // ~25 entries: 400 bytes on ESP32, 600 on a 64 bit PC
      filter["utc_offset_seconds"] = true;
      JsonObject fc = filter.createNestedObject("current");
      fc["time"] = true; fc["temperature_2m"] = true; fc["apparent_temperature"] = true;
      fc["wind_speed_10m"] = true; fc["wind_direction_10m"] = true;
      fc["weather_code"] = true; fc["is_day"] = true;
      JsonObject fh = filter.createNestedObject("hourly");
      fh["time"] = true; fh["temperature_2m"] = true; fh["weather_code"] = true;
      fh["is_day"] = true; fh["relative_humidity_2m"] = true; fh["pressure_msl"] = true;
      JsonObject fd = filter.createNestedObject("daily");
      fd["time"] = true; fd["temperature_2m_max"] = true; fd["temperature_2m_min"] = true;
      fd["precipitation_sum"] = true; fd["precipitation_probability_max"] = true;
      fd["sunrise"] = true; fd["sunset"] = true;

      String uri;
      uri += "/v1/forecast?latitude=" + String((float) LATITUDE, 5);
      uri += "&longitude=" + String((float) LONGITUDE, 5);
      uri += "&current=temperature_2m,apparent_temperature,wind_speed_10m,wind_direction_10m,weather_code,is_day";
      uri += "&hourly=temperature_2m,weather_code,is_day,relative_humidity_2m,pressure_msl";
      uri += "&daily=temperature_2m_max,temperature_2m_min,precipitation_sum,precipitation_probability_max,sunrise,sunset";
      uri += "&wind_speed_unit=kmh&timeformat=unixtime&forecast_days=8";
      uri += "&timezone=" + String(WEATHER_TIMEZONE);

      WiFiClientSecure client;
      HTTPClient       http;

      client.setInsecure();   // public weather data: no certificate pinning
      http.useHTTP10(true);   // no chunked transfer, so the stream can be parsed directly
      http.setTimeout(15000);
      http.begin(client, OPEN_METEO_SRV, OPEN_METEO_PORT, uri, true);

      int httpCode = http.GET();

      if (httpCode != HTTP_CODE_OK) {
         Serial.printf("GetWeather failed, http code: %d (%s)\n", httpCode, http.errorToString(httpCode).c_str());
         http.end();
         return false;
      }

      DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
      http.end();
      if (error) {
         Serial.print(F("deserializeJson() failed: "));
         Serial.println(error.c_str());
         return false;
      }
      return true;
   }

   /* Fill from the json data into the internal data. */
   bool Fill(const JsonObject &root)
   {
      Clear();

      time_t nowUtc = (time_t) (root["current"]["time"] | 0L);
      if (nowUtc == 0) {
         return false;   // not an Open-Meteo answer (error object, empty body ...)
      }
      currentTimeOffset = root["utc_offset_seconds"] | 0;
      currentTime       = LocalTime(nowUtc);
      winddir           = root["current"]["wind_direction_10m"] | 0.0f;
      windspeed         = root["current"]["wind_speed_10m"] | 0.0f;
      currentTemp       = root["current"]["temperature_2m"] | 0.0f;
      currentThermalComfort = root["current"]["apparent_temperature"] | currentTemp;

      JsonArray dTime = root["daily"]["time"];
      JsonArray hTime = root["hourly"]["time"];
      JsonArray hTemp = root["hourly"]["temperature_2m"];
      JsonArray hCode = root["hourly"]["weather_code"];
      JsonArray hDay  = root["hourly"]["is_day"];
      JsonArray hHum  = root["hourly"]["relative_humidity_2m"];
      JsonArray hPres = root["hourly"]["pressure_msl"];
      if (dTime.isNull() || hTime.isNull() || hTemp.isNull()) {
         return false;
      }
      sunrise = LocalTime(root["daily"]["sunrise"][0] | 0L);
      sunset  = LocalTime(root["daily"]["sunset"][0]  | 0L);

      // Hourly series start at 00:00 of today: find the entry of the current hour.
      int hourCount = hTime.size();
      int h0        = 0;
      while (h0 + 1 < hourCount && (time_t) (hTime[h0 + 1] | 0L) <= nowUtc) {
         h0++;
      }

      char buf[4];
      for (int i = 0; i < MAX_HOURLY; i++) {
         int k = h0 + i;

         if (i == 0) {
            int code = root["current"]["weather_code"] | 0;
            hourlyTime[0]    = currentTime;
            hourlyMaxTemp[0] = currentTemp;
            hourlyIcon[0]    = WmoToIcon(code, (root["current"]["is_day"] | 1) != 0, buf);
            hourlyMain[0]    = WmoToText(code);
         } else if (k < hourCount) {
            int code = hCode[k] | 0;
            hourlyTime[i]    = LocalTime((time_t) (hTime[k] | 0L));
            hourlyMaxTemp[i] = hTemp[k] | 0.0f;
            hourlyIcon[i]    = WmoToIcon(code, (hDay[k] | 1) != 0, buf);
            hourlyMain[i]    = WmoToText(code);
         } else {
            hourlyIcon[i]    = "";   // unknown icon
         }
      }

      JsonArray dMax  = root["daily"]["temperature_2m_max"];
      JsonArray dMin  = root["daily"]["temperature_2m_min"];
      JsonArray dRain = root["daily"]["precipitation_sum"];
      JsonArray dProb = root["daily"]["precipitation_probability_max"];
      int       days  = min((int) dTime.size(), MAX_FORECAST);

      for (int d = 0; d < days; d++) {
         time_t dayStart = (time_t) (dTime[d] | 0L);   // local midnight (as UTC instant)
         float  sumHum = 0, sumPres = 0;
         int    nHum = 0, nPres = 0;

         forecastTime[d]    = LocalTime(dayStart);
         forecastMaxTemp[d] = dMax[d]  | 0.0f;
         forecastMinTemp[d] = dMin[d]  | 0.0f;
         forecastRain[d]    = dRain[d] | 0.0f;
         if (forecastRain[d] > maxRain) {
            maxRain = (int) ceil(forecastRain[d]);
         }
         // humidity and pressure: mean of the hours of this day
         for (int k = 0; k < hourCount; k++) {
            time_t t = (time_t) (hTime[k] | 0L);
            if (t >= dayStart && t < dayStart + 86400) {
               if (!hHum[k].isNull())  { sumHum  += hHum[k].as<float>();  nHum++;  }
               if (!hPres[k].isNull()) { sumPres += hPres[k].as<float>(); nPres++; }
            }
         }
         forecastHumidity[d] = nHum  ? sumHum  / nHum  : 0;
         forecastPressure[d] = nPres ? sumPres / nPres : 0;
      }
      rainProbToday = dProb[0] | 0;
      ComputeAxes(days);
      valid = true;
      return true;
   }

   /* Choose the graph ranges from the data (a mountain village has no fixed ranges). */
   void ComputeAxes(int days)
   {
      float tMin = 1000, tMax = -1000, pMin = 10000, pMax = 0;

      for (int d = 0; d < days; d++) {
         tMin = min(tMin, forecastMinTemp[d]);
         tMax = max(tMax, forecastMaxTemp[d]);
         if (forecastPressure[d] > 0) {
            pMin = min(pMin, forecastPressure[d]);
            pMax = max(pMax, forecastPressure[d]);
         }
      }
      if (days == 0) {
         return;
      }
      tempAxisMin = (int) floor(tMin / 5.0f) * 5;
      tempAxisMax = (int) ceil (tMax / 5.0f) * 5;
      if (tempAxisMax - tempAxisMin < 10) tempAxisMax = tempAxisMin + 10;

      if (pMax > 0) {
         pressureAxisMin = (int) floor((pMin - 1) / 10.0f) * 10;
         pressureAxisMax = (int) ceil ((pMax + 1) / 10.0f) * 10;
         if (pressureAxisMax - pressureAxisMin < 20) pressureAxisMax = pressureAxisMin + 20;
      }
   }

public:
   Weather()
   {
      Clear();
   }

   /* Clear the internal data. */
   void Clear()
   {
      valid             = false;
      currentTime       = 0;
      currentTimeOffset = 0;
      sunrise           = 0;
      sunset            = 0;
      winddir           = 0;
      windspeed         = 0;
      maxRain           = MIN_RAIN;
      rainProbToday     = 0;
      tempAxisMin       = -10;
      tempAxisMax       = 30;
      pressureAxisMin   = 980;
      pressureAxisMax   = 1040;
      currentTemp       = 0;
      currentThermalComfort = 0;
      memset(hourlyTime,       0, sizeof(hourlyTime));
      memset(hourlyMaxTemp,    0, sizeof(hourlyMaxTemp));
      memset(forecastTime,     0, sizeof(forecastTime));
      memset(forecastMaxTemp,  0, sizeof(forecastMaxTemp));
      memset(forecastMinTemp,  0, sizeof(forecastMinTemp));
      memset(forecastRain,     0, sizeof(forecastRain));
      memset(forecastHumidity, 0, sizeof(forecastHumidity));
      memset(forecastPressure, 0, sizeof(forecastPressure));
      for (int i = 0; i < MAX_HOURLY; i++) {
         hourlyMain[i] = "";
         hourlyIcon[i] = "";
      }
   }

   /* Start the request and the filling. */
   bool Get()
   {
      DynamicJsonDocument doc(40 * 1024);

      if (GetOpenMeteoJsonDoc(doc)) {
         return Fill(doc.as<JsonObject>());
      }
      return false;
   }
};
