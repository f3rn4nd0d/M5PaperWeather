/**
  * @file HomeAssistant.h
  *
  * Reads the state of a few Home Assistant entities with ONE request:
  * POST /api/template renders a Jinja dictionary that already holds every value we need.
  * Read-only. A failure only blanks the Home Assistant page, never the weather.
  */
#pragma once
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include "HAConfig.h"
#include "HASecrets.h"

inline bool HaConfigured() { return HA_TOKEN[0] != 0; }

// one Jinja dictionary, single quotes only so it can sit inside a JSON string
#define HA_ATTR(e, a) "state_attr('" e "','" a "')"
#define HA_ST(e)      "states('" e "')"
#define HA_TEMPLATE \
   "{{ {" \
   "'th_cur':"   HA_ATTR(HA_THERMOSTAT, "current_temperature") "," \
   "'th_set':"   HA_ATTR(HA_THERMOSTAT, "temperature") "," \
   "'th_mode':"  HA_ST(HA_THERMOSTAT) "," \
   "'th_act':"   HA_ATTR(HA_THERMOSTAT, "hvac_action") "," \
   "'car_bat':"  HA_ST(HA_CAR_BATTERY) "," \
   "'car_chg':"  HA_ST(HA_CAR_CHARGE) "," \
   "'car_plug':" HA_ST(HA_CAR_PLUG) "," \
   "'car_left':" HA_ST(HA_CAR_TIME_LEFT) "," \
   "'car_rng':"  HA_ST(HA_CAR_RANGE) "," \
   "'ev_st':"    HA_ST(HA_EV_STATUS) "," \
   "'ev_pw':"    HA_ST(HA_EV_POWER) "," \
   "'ev_pu':"    HA_ATTR(HA_EV_POWER, "unit_of_measurement") "," \
   "'ev_ses':"   HA_ST(HA_EV_SESSION) "," \
   "'dh_st':"    HA_ST(HA_DEHUM) "," \
   "'dh_hum':"   HA_ST(HA_DEHUM_HUMIDITY) "," \
   "'dh_tank':"  HA_ST(HA_DEHUM_TANK) "," \
   "'dh_filt':"  HA_ST(HA_DEHUM_FILTER) "," \
   "'vac_st':"   HA_ST(HA_VACUUM) "," \
   "'vac_bat':"  HA_ATTR(HA_VACUUM, "battery_level") "," \
   "'lit_st':"   HA_ST(HA_LITTER) "," \
   "'lit_n':"    HA_ST(HA_LITTER_COUNT) \
   "} | tojson }}"

class HAData
{
public:
   bool   valid = false;           //!< true when the last request worked

   float  thCur = NAN, thSet = NAN;
   String thMode, thAction;

   float  carBat = NAN, carRange = NAN, carMinLeft = NAN;
   String carCharge, carPlug;

   String evStatus;
   float  evKw = NAN, evSession = NAN;

   String dhState;
   float  dhHum = NAN;
   bool   dhTankFull = false, dhFilter = false;

   String vacState;
   float  vacBat = NAN;

   String litState;
   float  litCount = NAN;

protected:
   /* numeric value of a JSON number or numeric string; NAN for null / unavailable / unknown */
   static float Num(JsonVariantConst v)
   {
      if (v.is<float>() || v.is<int>()) return v.as<float>();
      if (v.is<const char *>()) {
         const char *s = v.as<const char *>();
         char       *end = NULL;
         float       f = strtof(s, &end);

         if (end != s && *end == 0) return f;
      }
      return NAN;
   }

   /* text of a JSON string, "" for null / unavailable / unknown */
   static String Str(JsonVariantConst v)
   {
      if (!v.is<const char *>()) return String("");
      String s = v.as<const char *>();

      if (s == "unavailable" || s == "unknown" || s == "None") return String("");
      return s;
   }

public:
   bool Get()
   {
      valid = false;
      if (!HaConfigured()) return false;

      WiFiClient client;
      HTTPClient http;

      http.setConnectTimeout(HA_TIMEOUT_MS);
      http.setTimeout(HA_TIMEOUT_MS);
      if (!http.begin(client, HA_URL_HOST, HA_URL_PORT, "/api/template")) return false;
      http.addHeader("Authorization", String("Bearer ") + HA_TOKEN);
      http.addHeader("Content-Type", "application/json");

      int code = http.POST(String("{\"template\":\"") + HA_TEMPLATE + "\"}");

      if (code != 200) {
         Serial.printf("HomeAssistant failed, http code: %d (%s)\n", code, http.errorToString(code).c_str());
         http.end();
         return false;
      }
      String                body = http.getString();
      StaticJsonDocument<1536> doc;

      http.end();
      if (deserializeJson(doc, body)) {
         Serial.println("HomeAssistant: bad json");
         return false;
      }
      thCur = Num(doc["th_cur"]);  thSet = Num(doc["th_set"]);
      thMode = Str(doc["th_mode"]); thAction = Str(doc["th_act"]);
      carBat = Num(doc["car_bat"]); carRange = Num(doc["car_rng"]); carMinLeft = Num(doc["car_left"]);
      carCharge = Str(doc["car_chg"]); carPlug = Str(doc["car_plug"]);
      evStatus = Str(doc["ev_st"]);
      evKw = Num(doc["ev_pw"]);
      if (!isnan(evKw) && Str(doc["ev_pu"]) == "W") evKw /= 1000.0;
      evSession = Num(doc["ev_ses"]);
      dhState = Str(doc["dh_st"]); dhHum = Num(doc["dh_hum"]);
      dhTankFull = Str(doc["dh_tank"]) == "on"; dhFilter = Str(doc["dh_filt"]) == "on";
      vacState = Str(doc["vac_st"]); vacBat = Num(doc["vac_bat"]);
      litState = Str(doc["lit_st"]); litCount = Num(doc["lit_n"]);
      valid = true;
      return true;
   }
};
