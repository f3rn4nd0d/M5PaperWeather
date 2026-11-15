/**
  * @file HAConfig.h
  *
  * Home Assistant page: address and entity ids (same ones as retro-v4).
  * The access token is NOT here: it lives in HASecrets.h (git-ignored, see HASecrets.example.h).
  */
#pragma once

#define HA_URL_HOST        "192.168.68.60"   // Home Assistant on the LAN (plain http, port 80 as in retro-v4)
#define HA_URL_PORT        80
#define HA_TIMEOUT_MS      6000

// Thermostat (Tuya Local)
#define HA_THERMOSTAT      "climate.thermostat_2"
// Honda e (My Honda+)
#define HA_CAR_BATTERY     "sensor.honda_e_bateria"
#define HA_CAR_CHARGE      "sensor.honda_e_estado_de_carga"
#define HA_CAR_PLUG        "sensor.honda_e_estado_del_enchufe"
#define HA_CAR_TIME_LEFT   "sensor.honda_e_tiempo_de_carga_restante"
#define HA_CAR_RANGE       "sensor.honda_e_autonomia_total"
// EV charger (OCPP)
#define HA_EV_STATUS       "sensor.charger_status_connector"
#define HA_EV_POWER        "sensor.charger_power_active_import"
#define HA_EV_SESSION      "sensor.charger_energy_session"
// Dehumidifier of the games room (Midea AC LAN)
#define HA_DEHUM           "humidifier.deshumidificador_dehumidifier"
#define HA_DEHUM_HUMIDITY  "sensor.deshumidificador_current_humidity"
#define HA_DEHUM_TANK      "binary_sensor.deshumidificador_tank_status"
#define HA_DEHUM_FILTER    "binary_sensor.deshumidificador_filter_needs_cleaning"
// Roborock vacuum
#define HA_VACUUM          "vacuum.roborock_qrevo_edge_series"
// Litter box
#define HA_LITTER          "sensor.arenero_autolimpiable_snow_estado"
#define HA_LITTER_COUNT    "sensor.arenero_autolimpiable_snow_tiempos_de_excrecion_dia"
