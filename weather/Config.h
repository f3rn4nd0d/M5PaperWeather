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
  * @file Config.h
  * 
  * configuration information
  */
#pragma once

#define VERSION          "Version 2.0"
#define CITY_NAME        "Grajera, Segovia"

// Location: Grajera (Segovia), 1014 m. Fine tune with the exact coordinates of the house if you want.
#define LATITUDE         41.38150
#define LONGITUDE        -3.61000

// Weather data: Open-Meteo, free and without API key (data licence CC BY 4.0, attribution: open-meteo.com)
#define OPEN_METEO_SRV   "api.open-meteo.com"
#define OPEN_METEO_PORT  443
#define WEATHER_TIMEZONE "Europe/Madrid"

// Minutes to wait before a new try when wifi or the weather download fails
#define RETRY_MINUTES    10

#define WIFI_SSID        "your wifi ssid"
#define WIFI_PW          "your wifi password"
