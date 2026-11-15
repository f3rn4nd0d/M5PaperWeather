// Host stand-in for weather/Data.h + the Arduino / M5EPD / Utils pieces that Display.h uses.
// Only for scripts/preview.sh: lets Display.h be compiled on the Mac and drawn to a PNG.
#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>

#ifndef PI
#define PI 3.14159265358979
#endif
#define RETRY_MINUTES 10
#define WHITE 0
#define BLACK 15
#define TL_DATUM 0
#define UPDATE_MODE_GC16 0
#define constrain(a, lo, hi) ((a) < (lo) ? (lo) : ((a) > (hi) ? (hi) : (a)))
inline void delay(int) {}
template <class T> T max(T a, T b) { return a > b ? a : b; }

struct String {
   std::string s;
   String() {}
   String(const char *c) : s(c) {}
   String(const std::string &c) : s(c) {}
   String(int v) : s(std::to_string(v)) {}
   String(float v) : s(std::to_string(v)) {}
   const char *c_str() const { return s.c_str(); }
   size_t length() const { return s.size(); }
   int indexOf(const char *o) const { size_t i = s.find(o); return i == std::string::npos ? -1 : (int) i; }
   bool operator==(const char *o) const { return s == o; }
   String operator+(const String &o) const { return String(s + o.s); }
   String operator+(const char *o) const { return String(s + o); }
   friend String operator+(const char *a, const String &b) { return String(std::string(a) + b.s); }
};

struct SerialStub { void println(const char *m) { fprintf(stderr, "%s\n", m); } };
static SerialStub Serial;

// --- screen ---------------------------------------------------------------------------
static uint8_t SCREEN[960 * 540];

struct M5EPD_Canvas {
   enum { G0 = 0, G15 = 15 };
   std::vector<uint8_t> buf; int w = 0, h = 0;
   M5EPD_Canvas(void *) {}
   void createCanvas(int W, int H) { w = W; h = H; buf.assign(W * H, 0); }
   void deleteCanvas() { buf.clear(); w = h = 0; }
   void fillCanvas(int c) { std::fill(buf.begin(), buf.end(), c); }
   void setTextSize(int) {} void setTextColor(int, int) {} void setTextDatum(int) {}
   void drawPixel(int x, int y, uint32_t c) { if (x >= 0 && y >= 0 && x < w && y < h) buf[y * w + x] = c; }
   void drawLine(int x0, int y0, int x1, int y1, uint32_t c) {
      int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, e = dx + dy;
      for (;;) { drawPixel(x0, y0, c); if (x0 == x1 && y0 == y1) break; int e2 = 2 * e;
         if (e2 >= dy) { e += dy; x0 += sx; } if (e2 <= dx) { e += dx; y0 += sy; } }
   }
   void fillRect(int x, int y, int W, int H, uint32_t c) { for (int j = 0; j < H; j++) for (int i = 0; i < W; i++) drawPixel(x + i, y + j, c); }
   void drawRect(int x, int y, int W, int H, uint32_t c) { drawLine(x, y, x + W - 1, y, c); drawLine(x, y + H - 1, x + W - 1, y + H - 1, c); drawLine(x, y, x, y + H - 1, c); drawLine(x + W - 1, y, x + W - 1, y + H - 1, c); }
   void fillCircle(int cx, int cy, int r, uint32_t c) { for (int j = -r; j <= r; j++) for (int i = -r; i <= r; i++) if (i * i + j * j <= r * r) drawPixel(cx + i, cy + j, c); }
   void drawCircle(int cx, int cy, int r, uint32_t c) { for (int j = -r - 1; j <= r + 1; j++) for (int i = -r - 1; i <= r + 1; i++) { int d = i * i + j * j; if (d <= r * r + r && d >= r * r - r) drawPixel(cx + i, cy + j, c); } }
   bool inRound(int i, int j, int W, int H, int r) { int cx = std::min(std::max(i, r), W - 1 - r), cy = std::min(std::max(j, r), H - 1 - r); return (i - cx) * (i - cx) + (j - cy) * (j - cy) <= r * r; }
   void fillRoundRect(int x, int y, int W, int H, int r, uint32_t c) { for (int j = 0; j < H; j++) for (int i = 0; i < W; i++) if (inRound(i, j, W, H, r)) drawPixel(x + i, y + j, c); }
   void drawRoundRect(int x, int y, int W, int H, int r, uint32_t c) { for (int j = 0; j < H; j++) for (int i = 0; i < W; i++) if (inRound(i, j, W, H, r) && !inRound(i - 1, j - 1, W - 2, H - 2, r - 1)) drawPixel(x + i, y + j, c); }
   void fillTriangle(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c) {
      int minx = floor(std::min({x0, x1, x2})), maxx = ceil(std::max({x0, x1, x2})), miny = floor(std::min({y0, y1, y2})), maxy = ceil(std::max({y0, y1, y2}));
      auto e = [](float ax, float ay, float bx, float by, float px, float py) { return (px - ax) * (by - ay) - (py - ay) * (bx - ax); };
      for (int y = miny; y <= maxy; y++) for (int x = minx; x <= maxx; x++) { float a = e(x0, y0, x1, y1, x, y), b = e(x1, y1, x2, y2, x, y), d = e(x2, y2, x0, y0, x, y);
         if ((a >= 0 && b >= 0 && d >= 0) || (a <= 0 && b <= 0 && d <= 0)) drawPixel(x, y, c); }
   }
   void pushCanvas(int px, int py, int) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) if (px + i < 960 && py + j < 540) SCREEN[(py + j) * 960 + px + i] = buf[j * w + i]; }
};

struct rtc_date_t { int day, mon, year; };
struct { struct { void getDate(rtc_date_t *d) { d->day = 15; d->mon = 11; d->year = 2026; } } RTC; void *EPD = nullptr; } M5;

// --- time helpers (TimeLib subset) ----------------------------------------------------
inline struct tm TM(time_t t) { struct tm r; gmtime_r(&t, &r); return r; }
inline int weekday(time_t t) { return TM(t).tm_wday + 1; }
inline int day(time_t t) { return TM(t).tm_mday; }
inline int month(time_t t) { return TM(t).tm_mon + 1; }
inline int hour(time_t t) { return TM(t).tm_hour; }
inline int minute(time_t t) { return TM(t).tm_min; }
inline time_t GetRTCTime() { return 1794139440; }   // 2026-11-15 12:04 (a Sunday)
inline String getHourString(time_t t) { char b[8]; sprintf(b, "%02d", hour(t)); return b; }
inline String getHourMinString(time_t t) { char b[8]; sprintf(b, "%02d:%02d", hour(t), minute(t)); return b; }
inline int WifiGetRssiAsQualityInt(int r) { return r <= -100 ? 0 : r >= -50 ? 100 : 2 * (r + 100); }
inline int JulianDate(int d, int m, int y) { int yy = y - (int)((12 - m) / 10), mm = m + 9; if (mm >= 12) mm -= 12;
   int k1 = (int)(365.25 * (yy + 4712)), k2 = (int)(30.6001 * mm + 0.5), k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38, j = k1 + k2 + d + 59 + 1; if (j > 2299160) j -= k3; return j; }
inline double NormalizedMoonPhase(int d, int m, int y) { double p = (JulianDate(d, m, y) + 4.867) / 29.53059; return p - (int) p; }

// --- data -----------------------------------------------------------------------------
struct Weather {
   bool valid = true;
   time_t sunrise, sunset;
   float winddir = 225, windspeed = 14, currentTemp = 14.3, currentThermalComfort = 12.1;
   time_t hourlyTime[24]; float hourlyMaxTemp[24]; String hourlyMain[24]; String hourlyIcon[24];
   int maxRain = 8, rainProbToday = 20, tempAxisMin = -2, tempAxisMax = 20, pressureAxisMin = 1010, pressureAxisMax = 1030;
   time_t forecastTime[8]; float forecastMaxTemp[8], forecastMinTemp[8], forecastRain[8], forecastHumidity[8], forecastPressure[8];
   Weather() {
      sunrise = 1794139440 - 12 * 3600 + 4 * 3600 + 20 * 60 - 3600 * 0 + 0; sunset = sunrise + 10 * 3600 + 23 * 60;
      const char *icons[] = {"03d","02d","01d","01d","03d","04d","10d","10n"};
      for (int i = 0; i < 24; i++) { hourlyTime[i] = 1794139440 + i * 3600; hourlyMaxTemp[i] = 14 + 4 * sin(i / 3.0); hourlyMain[i] = "Poco nuboso"; hourlyIcon[i] = icons[(i / 3) % 8]; }
      float mx[] = {18, 17, 15, 12, 9, 10, 13, 16}, mn[] = {6, 7, 4, 1, -1, 0, 3, 5}, rn[] = {0, 0.4, 3, 8, 5, 1, 0, 0.2}, hu[] = {62, 66, 78, 90, 85, 70, 60, 58}, pr[] = {1021, 1019, 1014, 1012, 1016, 1022, 1026, 1024};
      for (int i = 0; i < 8; i++) { forecastTime[i] = 1794139440 - 12 * 3600 + i * 86400; forecastMaxTemp[i] = mx[i]; forecastMinTemp[i] = mn[i]; forecastRain[i] = rn[i]; forecastHumidity[i] = hu[i]; forecastPressure[i] = pr[i]; }
   }
};
// --- Home Assistant mock ----------------------------------------------------------------
inline bool HaConfigured() { return true; }
struct HAData {
   bool valid = true;
   float thCur = 21.5, thSet = 22, carBat = 78, carRange = 164, carMinLeft = 134, evKw = 7.2, evSession = 12.4, dhHum = 62, vacBat = 87, litCount = 3;
   String thMode = "heat", thAction = "heating", carCharge = "charging", carPlug = "plugged_in", evStatus = "Charging",
          dhState = "on", vacState = "docked", litState = "Disponible";
   bool dhTankFull = false, dhFilter = true;
};
struct MyData {
   uint8_t page = 0;
   HAData ha;
   int wifiRSSI = -62, batteryCapacity = 78, sht30Temperatur = 21, sht30Humidity = 48;
   time_t moonRise = 1794139440 + 2 * 3600 + 13 * 60 - 12 * 3600, moonSet = 1794139440 + 2 * 3600 + 40 * 60;
   Weather weather;
};
