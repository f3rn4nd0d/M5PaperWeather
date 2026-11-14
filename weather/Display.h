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
  * @file Display.h
  * 
  * Main class for drawing the content to the e-paper display.
  */
#pragma once
#include "Data.h"
#include "Icons.h"
#include "Logo.h"
#include "LogoRetro.h"
#include "Font.h"

/*
 * Layout (960 x 540), inspired by Apple's weather: one big number, rounded tiles, no frames,
 * no section titles, one typeface (Inter, see scripts/make_font.py) and four ink levels.
 *
 *   header  : logos .......................... wifi battery
 *   hero    : date / temperature / icon        | wind  sun
 *                                              | moon  indoor
 *   hourly  : 8 slots, every 3 hours
 *   graphs  : temperature, rain, humidity, pressure (8 days)
 */
static const int M        = 32;    //!< Outer margin
static const int GAP      = 12;    //!< Gap between tiles
static const int TILE_W   = 250;   //!< Width of a small tile
static const int TILE_H   = 102;   //!< Height of a small tile
static const int TILE_X0  = 416;   //!< Left column of the tiles
static const int TILE_X1  = TILE_X0 + TILE_W + GAP;
static const int TILE_Y0  = 56;
static const int TILE_Y1  = TILE_Y0 + TILE_H + GAP;
static const int HOURLY_Y = 282;
static const int HOURLY_H = 128;
static const int GRAPH_Y  = 420;

static const int LOGO_XH_TOP    = 4;    //!< x-height band of the Casa Grajera bitmap (see Logo.h)
static const int LOGO_XH_BOTTOM = 17;

static const int INK      = 15;    //!< Black
static const int INK_MID  = 8;     //!< Secondary text
static const int INK_SOFT = 5;     //!< Hairlines and the second graph line
static const int TILE_BG  = 1;     //!< Very light grey behind the tiles

M5EPD_Canvas canvas(&M5.EPD); // Main canvas of the e-paper

/* Main class for drawing the content to the e-paper display. */
class WeatherDisplay
{
protected:
   MyData &myData; //!< Reference to the global data
   int     maxX;   //!< Max width of the e-paper
   int     maxY;   //!< Max height of the e-paper

protected:
   // typography
   const GlyphInfo *FindGlyph(const FontInfo &font, uint16_t cp);
   int    TextWidth(const FontInfo &font, const String &text);
   void   DrawText(const FontInfo &font, int x, int baseline, const String &text, int ink = INK, int align = 0);

   // small helpers
   void   DrawCircle(int32_t x, int32_t y, int32_t r, uint32_t color, int32_t degFrom = 0, int32_t degTo = 360);
   void   DrawLine2(int x0, int y0, int x1, int y1, int ink);
   void   DrawTile(int x, int y, int dx, int dy);
   void   DrawIcon(int x, int y, const uint16_t *icon, int dx = 64, int dy = 64, bool highContrast = false);
   void   DrawWeatherIcon(int x, int y, const String &icon);
   void   DrawBitmap(int x, int y, const uint8_t *data, int dx, int dy);

   // header
   void   DrawHead();
   void   DrawRSSI(int x, int y);
   void   DrawBattery(int x, int y);

   // hero
   void   DrawHero(int x, int y, int dx, int dy);
   void   DrawNoData(int x, int y, int dx, int dy);

   // tiles
   void   DrawWindTile(int x, int y);
   void   DrawSunTile(int x, int y);
   void   DrawMoonTile(int x, int y);
   void   DrawIndoorTile(int x, int y);
   void   DrawMoonDisc(int cx, int cy, int r, double phase);
   void   DrawTriangle(int x, int y, int size, bool up, int ink);

   // forecast
   void   DrawHourly(int x, int y, int dx, int dy, Weather &weather, int index);
   void   DrawGraph(int x, int y, int dx, int dy, String title, String extra, int yMin, int yMax,
                    float values[], float values2[] = NULL, bool bars = false);

   String DayLetter(int index);
   String LongDate();

public:
   WeatherDisplay(MyData &md, int x = 960, int y = 540)
      : myData(md)
      , maxX(x)
      , maxY(y)
   {
   }

   void Show();

   void ShowM5PaperInfo();
};

/* ------------------------------------------------------------------ typography */

/* Find a glyph of a pre-rendered font */
const GlyphInfo *WeatherDisplay::FindGlyph(const FontInfo &font, uint16_t cp)
{
   for (int i = 0; i < font.count; i++) {
      if (font.glyphs[i].cp == cp) return &font.glyphs[i];
   }
   return NULL;
}

/* Next code point of an UTF-8 string, advances the pointer */
static uint16_t NextCodePoint(const char *&p)
{
   uint8_t c = (uint8_t) *p++;

   if (c < 0x80) return c;
   if ((c & 0xE0) == 0xC0 && *p) return ((c & 0x1F) << 6) | ((uint8_t) *p++ & 0x3F);
   if ((c & 0xF0) == 0xE0 && p[0] && p[1]) {
      uint16_t cp = ((c & 0x0F) << 12) | (((uint8_t) p[0] & 0x3F) << 6) | ((uint8_t) p[1] & 0x3F);
      p += 2;
      return cp;
   }
   return '?';
}

/* Width in pixels of a text */
int WeatherDisplay::TextWidth(const FontInfo &font, const String &text)
{
   int         w = 0;
   const char *p = text.c_str();

   while (*p) {
      const GlyphInfo *g = FindGlyph(font, NextCodePoint(p));

      if (g) w += g->adv;
   }
   return w;
}

/* Draw a text with its baseline at "baseline". align: 0 = left, 1 = centre, 2 = right of x */
void WeatherDisplay::DrawText(const FontInfo &font, int x, int baseline, const String &text, int ink, int align)
{
   if (align == 1) x -= TextWidth(font, text) / 2;
   if (align == 2) x -= TextWidth(font, text);

   const char *p = text.c_str();

   while (*p) {
      const GlyphInfo *g = FindGlyph(font, NextCodePoint(p));

      if (!g) continue;
      const uint8_t *bits = font.bits + g->off;

      for (int yi = 0; yi < g->h; yi++) {
         for (int xi = 0; xi < g->w; xi++) {
            int v = bits[yi * g->w + xi];

            if (v > 0) canvas.drawPixel(x + g->xo + xi, baseline + g->yo + yi, ink == INK ? v : (v * ink + 7) / 15);
         }
      }
      x += g->adv;
   }
}

/* ------------------------------------------------------------------ helpers */

/* Draw a circle with optional start and end point */
void WeatherDisplay::DrawCircle(int32_t x, int32_t y, int32_t r, uint32_t color, int32_t degFrom /* = 0 */, int32_t degTo /* = 360 */)
{
   for (int i = degFrom; i < degTo; i++) {
      double radians = i * PI / 180;
      double px      = x + r * cos(radians);
      double py      = y + r * sin(radians);

      canvas.drawPixel(px, py, color);
   }
}

/* A line two pixels thick */
void WeatherDisplay::DrawLine2(int x0, int y0, int x1, int y1, int ink)
{
   canvas.drawLine(x0, y0, x1, y1, ink);
   canvas.drawLine(x0, y0 + 1, x1, y1 + 1, ink);
}

/* Rounded light-grey tile */
void WeatherDisplay::DrawTile(int x, int y, int dx, int dy)
{
   canvas.fillRoundRect(x, y, dx, dy, 16, TILE_BG);
}

/* Draw one icon from the binary data */
void WeatherDisplay::DrawIcon(int x, int y, const uint16_t *icon, int dx /*= 64*/, int dy /*= 64*/, bool highContrast /*= false*/)
{
   for (int yi = 0; yi < dy; yi++) {
      for (int xi = 0; xi < dx; xi++) {
         uint16_t pixel = icon[yi * dx + xi];

         if (highContrast) {
            if (15 - (pixel / 4096) > 0) canvas.drawPixel(x + xi, y + yi, M5EPD_Canvas::G15);
         } else {
            canvas.drawPixel(x + xi, y + yi, 15 - (pixel / 4096));
         }
      }
   }
}

/* Draw the weather icon for an openweathermap style code (01d, 10n ...) */
void WeatherDisplay::DrawWeatherIcon(int x, int y, const String &icon)
{
        if (icon == "01d") DrawIcon(x, y, (uint16_t *) image_data_01d, 64, 64, true);
   else if (icon == "01n") DrawIcon(x, y, (uint16_t *) image_data_03n, 64, 64, true);
   else if (icon == "02d") DrawIcon(x, y, (uint16_t *) image_data_02d, 64, 64, true);
   else if (icon == "02n") DrawIcon(x, y, (uint16_t *) image_data_02n, 64, 64, true);
   else if (icon == "03d") DrawIcon(x, y, (uint16_t *) image_data_03d, 64, 64, true);
   else if (icon == "03n") DrawIcon(x, y, (uint16_t *) image_data_03n, 64, 64, true);
   else if (icon == "04d") DrawIcon(x, y, (uint16_t *) image_data_04d, 64, 64, true);
   else if (icon == "04n") DrawIcon(x, y, (uint16_t *) image_data_03n, 64, 64, true);
   else if (icon == "09d") DrawIcon(x, y, (uint16_t *) image_data_09d, 64, 64, true);
   else if (icon == "09n") DrawIcon(x, y, (uint16_t *) image_data_09n, 64, 64, true);
   else if (icon == "10d") DrawIcon(x, y, (uint16_t *) image_data_10d, 64, 64, true);
   else if (icon == "10n") DrawIcon(x, y, (uint16_t *) image_data_03n, 64, 64, true);
   else if (icon == "11d") DrawIcon(x, y, (uint16_t *) image_data_11d, 64, 64, true);
   else if (icon == "11n") DrawIcon(x, y, (uint16_t *) image_data_11n, 64, 64, true);
   else if (icon == "13d") DrawIcon(x, y, (uint16_t *) image_data_13d, 64, 64, true);
   else if (icon == "13n") DrawIcon(x, y, (uint16_t *) image_data_13n, 64, 64, true);
   else if (icon == "50d") DrawIcon(x, y, (uint16_t *) image_data_50d, 64, 64, true);
   else if (icon == "50n") DrawIcon(x, y, (uint16_t *) image_data_50n, 64, 64, true);
   else DrawIcon(x, y, (uint16_t *) image_data_unknown, 64, 64, true);
}

/* Draw a logo bitmap (4-bit ink levels, one byte per pixel) */
void WeatherDisplay::DrawBitmap(int x, int y, const uint8_t *data, int dx, int dy)
{
   for (int yi = 0; yi < dy; yi++) {
      for (int xi = 0; xi < dx; xi++) {
         uint8_t ink = data[yi * dx + xi];

         if (ink > 0) canvas.drawPixel(x + xi, y + yi, ink);
      }
   }
}

/* Small solid triangle, used as sunrise / sunset / moonrise / moonset arrow */
void WeatherDisplay::DrawTriangle(int x, int y, int size, bool up, int ink)
{
   if (up) canvas.fillTriangle(x, y + size, x + size, y + size, x + size / 2, y, ink);
   else    canvas.fillTriangle(x, y, x + size, y, x + size / 2, y + size, ink);
}

/* Weekday letter below the graphs: L M X J V S D */
String WeatherDisplay::DayLetter(int index)
{
   static const char *letters[8] = {"", "D", "L", "M", "X", "J", "V", "S"};
   time_t t = myData.weather.forecastTime[index];

   if (t == 0) return String(index);
   return String(letters[weekday(t)]);
}

/* Date of the RTC: "Domingo 15 nov  12:04" */
String WeatherDisplay::LongDate()
{
   static const char *days[8]   = {"", "Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};
   static const char *months[13] = {"", "ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sep", "oct", "nov", "dic"};
   time_t t = GetRTCTime();
   char   buf[16];

   sprintf(buf, "%02d:%02d", hour(t), minute(t));
   return String(days[weekday(t)]) + " " + String(day(t)) + " " + String(months[month(t)]) + "  " + String(buf);
}

/* ------------------------------------------------------------------ header */

/* Draw a the rssi value as circle parts */
void WeatherDisplay::DrawRSSI(int x, int y)
{
   int iQuality = WifiGetRssiAsQualityInt(myData.wifiRSSI);

   if (iQuality >= 80) DrawCircle(x + 12, y, 16, M5EPD_Canvas::G15, 225, 315);
   if (iQuality >= 40) DrawCircle(x + 12, y, 12, M5EPD_Canvas::G15, 225, 315);
   if (iQuality >= 20) DrawCircle(x + 12, y,  8, M5EPD_Canvas::G15, 225, 315);
   if (iQuality >= 10) DrawCircle(x + 12, y,  4, M5EPD_Canvas::G15, 225, 315);
   DrawCircle(x + 12, y, 2, M5EPD_Canvas::G15, 225, 315);
}

/* Draw a the battery icon */
void WeatherDisplay::DrawBattery(int x, int y)
{
   canvas.drawRoundRect(x, y, 34, 16, 4, INK);
   canvas.fillRect(x + 35, y + 5, 3, 6, INK);
   int w = constrain(myData.batteryCapacity, 0, 100) * 28 / 100;

   canvas.fillRoundRect(x + 3, y + 3, w, 10, 2, INK);
}

/* Header: both logos centred, wifi and battery on the right */
void WeatherDisplay::DrawHead()
{
   const int cy = 26;                                   // common centre line of the header
   // Casa Grajera: centre its x-height band (rows 4..17 of the bitmap), the "j" hangs below
   int lockupW = LOGO_W + 32 + RETRO_W;                  // both logos + divider, centred on the screen
   int lx      = (maxX - lockupW) / 2;

   DrawBitmap(lx, cy - (LOGO_XH_TOP + LOGO_XH_BOTTOM) / 2, LOGO_CASA_GRAJERA, LOGO_W, LOGO_H);
   canvas.drawLine(lx + LOGO_W + 16, cy - 11, lx + LOGO_W + 16, cy + 11, INK_SOFT);
   DrawBitmap(lx + LOGO_W + 32, cy - RETRO_H / 2, RETRO_MARK, RETRO_W, RETRO_H);

   int right = maxX - M;

   DrawBattery(right - 38, cy - 8);
   DrawText(FONT_CAP, right - 50, cy + 7, String(myData.batteryCapacity) + "%", INK_MID, 2);
   DrawRSSI(right - 130, cy + 12);
}

/* ------------------------------------------------------------------ hero */

/* Date, current temperature, icon, min / max and the short description */
void WeatherDisplay::DrawHero(int x, int y, int dx, int dy)
{
   Weather &w    = myData.weather;
   String   temp = String((int) round(w.currentTemp)) + "°";
   String   hi   = "Máx " + String((int) round(w.forecastMaxTemp[0])) + "°";
   String   lo   = "Mín " + String((int) round(w.forecastMinTemp[0])) + "°";
   String   desc = w.hourlyMain[0] + "  ·  sensación " + String((int) round(w.currentThermalComfort)) + "°";
   int      cx   = x + dx / 2;                        // everything is centred between the margin and the tiles

   DrawText(FONT_BODY, cx, y + 28, LongDate(), INK_MID, 1);

   // big number + (icon over max / min) as one block
   int numW  = TextWidth(FONT_HERO, temp);
   int sideW = max(64, max(TextWidth(FONT_BODY, hi), TextWidth(FONT_BODY, lo)));
   int gap   = 30;
   int left  = cx - (numW + gap + sideW) / 2;
   int sideX = left + numW + gap;

   DrawText(FONT_HERO, left, y + 172, temp);
   DrawWeatherIcon(sideX + (sideW - 64) / 2, y + 44, w.hourlyIcon[0]);
   DrawText(FONT_BODY, sideX, y + 140, hi);
   DrawText(FONT_BODY, sideX, y + 172, lo, INK_MID);

   DrawText(FONT_BODY, cx, y + 210, desc, INK_MID, 1);
}

/* Message shown instead of the weather when the download failed */
void WeatherDisplay::DrawNoData(int x, int y, int dx, int dy)
{
   int cx = x + dx / 2;

   DrawText(FONT_BODY, cx, y + 28, LongDate(), INK_MID, 1);
   DrawText(FONT_TITLE, cx, y + 96, "Sin datos del tiempo", INK, 1);
   DrawText(FONT_BODY, cx, y + 140, "No se pudo descargar la previsión.", INK_MID, 1);
   DrawText(FONT_BODY, cx, y + 172, "Nuevo intento en " + String(RETRY_MINUTES) + " minutos.", INK_MID, 1);
}

/* ------------------------------------------------------------------ tiles */

static const char *CompassName(float deg)
{
   static const char *names[8] = {"Norte", "Noreste", "Este", "Sureste", "Sur", "Suroeste", "Oeste", "Noroeste"};

   return names[((int) round(deg / 45.0)) % 8];
}

/* Wind: a small compass with the arrow pointing where the wind blows to, speed and origin.
 * Every tile measures its content and centres it, so nothing hugs the left edge. */
void WeatherDisplay::DrawWindTile(int x, int y)
{
   float  angle = myData.weather.winddir;
   String speed = String((int) round(myData.weather.windspeed));
   String name  = CompassName(angle);
   int    r     = 34;
   int    textW = max(TextWidth(FONT_TITLE, speed) + 6 + TextWidth(FONT_CAP, "km/h"), TextWidth(FONT_BODY, name));
   int    gap   = 20;
   int    left  = x + (TILE_W - (2 * r + gap + textW)) / 2;
   int    cx    = left + r, cy = y + TILE_H / 2;
   int    tx    = left + 2 * r + gap;
   double a     = angle * PI / 180.0;
   double ux    = sin(a), uy = -cos(a);              // unit vector towards where the wind comes from

   DrawTile(x, y, TILE_W, TILE_H);
   canvas.drawCircle(cx, cy, r, INK_SOFT);
   canvas.drawCircle(cx, cy, r - 1, INK_SOFT);
   for (int i = 0; i < 4; i++) {                     // N E S W ticks
      double t = i * PI / 2;

      canvas.drawLine(cx + (r - 8) * sin(t), cy - (r - 8) * cos(t), cx + (r - 1) * sin(t), cy - (r - 1) * cos(t), i == 0 ? INK : INK_SOFT);
   }
   // arrow: from the origin of the wind through the centre, head on the far side
   DrawLine2(cx + ux * (r - 12), cy + uy * (r - 12), cx - ux * 8, cy - uy * 8, INK);
   double hx = cx - ux * 17, hy = cy - uy * 17;      // tip
   double bx = cx - ux * 4,  by = cy - uy * 4;       // base centre
   canvas.fillTriangle(hx, hy, bx - uy * 7, by + ux * 7, bx + uy * 7, by - ux * 7, INK);

   DrawText(FONT_TITLE, tx, y + 52, speed);
   DrawText(FONT_CAP, tx + TextWidth(FONT_TITLE, speed) + 6, y + 52, "km/h", INK_MID);
   DrawText(FONT_BODY, tx, y + 84, name, INK_MID);
}

/* Sun: sunrise and sunset, the block (arrow + time) centred in the tile */
void WeatherDisplay::DrawSunTile(int x, int y)
{
   String rise = getHourMinString(myData.weather.sunrise);
   String set  = getHourMinString(myData.weather.sunset);
   int    timeW = max(TextWidth(FONT_TITLE, rise), TextWidth(FONT_TITLE, set));
   int    left  = x + (TILE_W - (16 + 22 + timeW)) / 2;

   DrawTile(x, y, TILE_W, TILE_H);
   DrawTriangle(left, y + 19, 16, true, INK);
   DrawText(FONT_TITLE, left + 38, y + 43, rise);
   DrawTriangle(left, y + 65, 16, false, INK_MID);
   DrawText(FONT_TITLE, left + 38, y + 87, set, INK_MID);
}

/* The moon disc: dark side grey, lit side white, drawn from the normalized phase (0 new, 0.5 full) */
void WeatherDisplay::DrawMoonDisc(int cx, int cy, int r, double phase)
{
   double c = cos(2.0 * PI * phase);

   canvas.fillCircle(cx, cy, r, 11);
   for (int dy = -r; dy <= r; dy++) {
      double hw = sqrt((double) r * r - (double) dy * dy);
      double t  = hw * c;                            // terminator
      int    x0 = (int) round(phase < 0.5 ? t : -hw);
      int    x1 = (int) round(phase < 0.5 ? hw : -t);

      if (x1 > x0) canvas.drawLine(cx + x0, cy + dy, cx + x1, cy + dy, 0);
   }
   canvas.drawCircle(cx, cy, r, INK);
}

/* Moon: phase drawing, name, illumination, moonrise and moonset (block centred in the tile) */
void WeatherDisplay::DrawMoonTile(int x, int y)
{
   static const char *names[8] = {"Nueva", "Creciente", "1er cuarto", "Gibosa +", "Llena", "Gibosa -", "3er cuarto", "Menguante"};
   rtc_date_t date;

   M5.RTC.getDate(&date);
   double phase = NormalizedMoonPhase(date.day, date.mon, date.year);
   int    illum = (int) round((1.0 - cos(2.0 * PI * phase)) / 2.0 * 100.0);
   String name  = names[((int) (phase * 8.0 + 0.5)) % 8];
   String rise  = getHourMinString(myData.moonRise);
   String set   = getHourMinString(myData.moonSet);
   int    r     = 24;
   int    rowW  = (9 + 5 + TextWidth(FONT_CAP, rise)) + 18 + (9 + 5 + TextWidth(FONT_CAP, set));
   int    textW = max(TextWidth(FONT_BODY, name), rowW);
   int    gap   = 18;
   int    left  = x + (TILE_W - (2 * r + gap + textW)) / 2;
   int    tx    = left + 2 * r + gap;
   int    x2    = tx + (9 + 5 + TextWidth(FONT_CAP, rise)) + 18;

   DrawTile(x, y, TILE_W, TILE_H);
   DrawMoonDisc(left + r, y + TILE_H / 2, r, phase);
   DrawText(FONT_BODY, tx, y + 34, name);
   DrawText(FONT_CAP, tx, y + 57, String(illum) + "%", INK_MID);
   DrawTriangle(tx, y + 72, 9, true, INK);
   DrawText(FONT_CAP, tx + 14, y + 83, rise);
   DrawTriangle(x2, y + 72, 9, false, INK_MID);
   DrawText(FONT_CAP, x2 + 14, y + 83, set, INK_MID);
}

/* Indoor: SHT30 temperature and humidity, a house and a drop as the only "titles".
 * Each column (symbol over value) is centred on itself and the pair is centred in the tile. */
void WeatherDisplay::DrawIndoorTile(int x, int y)
{
   String t  = String(myData.sht30Temperatur);
   String h  = String(myData.sht30Humidity);
   int    w1 = TextWidth(FONT_TITLE, t) + 3 + TextWidth(FONT_CAP, "°C");
   int    w2 = TextWidth(FONT_TITLE, h) + 3 + TextWidth(FONT_CAP, "%");
   int    gap = 36;
   int    x1 = x + (TILE_W - (w1 + gap + w2)) / 2;     // left of the temperature column
   int    x2 = x1 + w1 + gap;                          // left of the humidity column
   int    c1 = x1 + w1 / 2, c2 = x2 + w2 / 2;
   int    base = y + 86;

   DrawTile(x, y, TILE_W, TILE_H);
   // house
   canvas.fillTriangle(c1 - 11, y + 28, c1 + 11, y + 28, c1, y + 16, INK_MID);
   canvas.fillRect(c1 - 7, y + 28, 14, 10, INK_MID);
   // drop
   canvas.fillCircle(c2, y + 31, 6, INK_MID);
   canvas.fillTriangle(c2 - 6, y + 29, c2 + 6, y + 29, c2, y + 16, INK_MID);

   DrawText(FONT_TITLE, x1, base, t);
   DrawText(FONT_CAP, x1 + TextWidth(FONT_TITLE, t) + 3, base, "°C", INK_MID);
   DrawText(FONT_TITLE, x2, base, h);
   DrawText(FONT_CAP, x2 + TextWidth(FONT_TITLE, h) + 3, base, "%", INK_MID);
}

/* ------------------------------------------------------------------ forecast */

/* Draw one hourly weather information */
void WeatherDisplay::DrawHourly(int x, int y, int dx, int dy, Weather &weather, int index)
{
   time_t time = weather.hourlyTime[index];
   int    temp = (int) round(weather.hourlyMaxTemp[index]);
   String label = getHourString(time) + ":00";
   int    cx    = x + dx / 2;

   if (index == 0) {
      label = "Ahora";
   } else if (hour(time) < 3) {   // first slot of a new day: show the weekday
      static const char *dayNames[8] = {"", "Do", "Lu", "Ma", "Mi", "Ju", "Vi", "Sa"};
      label = String(dayNames[weekday(time)]) + " " + label;
   }
   DrawText(FONT_CAP, cx, y + 27, label, index == 0 ? INK : INK_MID, 1);
   DrawWeatherIcon(cx - 32, y + 36, weather.hourlyIcon[index]);
   DrawText(FONT_BODY, cx, y + 120, String(temp) + "°", INK, 1);
}

/* Draw a graph card: caption, y range, 8 days.  bars = rain bars, values2 = optional second line */
void WeatherDisplay::DrawGraph(int x, int y, int dx, int dy, String title, String extra, int yMin, int yMax,
                               float values[], float values2[], bool bars)
{
   int px0 = x + 14 + 24, px1 = x + dx - 18;
   int py0 = y + 42,      py1 = y + dy - 24;
   int n   = 8;

   if (yMax <= yMin) yMax = yMin + 1;
   DrawTile(x, y, dx, dy);
   DrawText(FONT_CAP, x + 14, y + 26, title, INK);
   if (extra.length() > 0) DrawText(FONT_CAP, x + dx - 14, y + 26, extra, INK_MID, 2);

   DrawText(FONT_TINY, px0 - 6, py0 + 5,  String(yMax), INK_MID, 2);
   DrawText(FONT_TINY, px0 - 6, py1 + 1,  String(yMin), INK_MID, 2);
   canvas.drawLine(px0, py1 + 2, px1, py1 + 2, INK_SOFT);                    // baseline

   if (!bars && yMin < 0 && yMax > 0) {                                      // zero line, dashed
      int yz = py1 - (int) ((0.0 - yMin) * (py1 - py0) / (yMax - yMin));

      for (int xd = px0; xd < px1 - 6; xd += 8) canvas.drawLine(xd, yz, xd + 3, yz, INK_SOFT);
   }
   for (int s = 0; s < 2; s++) {
      float *v = (s == 0) ? values : values2;
      int    oldX = 0, oldY = 0;

      if (v == NULL) continue;
      for (int i = 0; i < n; i++) {
         int xPos = px0 + 6 + i * (px1 - px0 - 12) / (n - 1);
         int yPos = py1 - (int) round((v[i] - yMin) * (py1 - py0) / (float) (yMax - yMin));

         if (yPos > py1) yPos = py1;
         if (yPos < py0) yPos = py0;
         if (bars) {
            if (py1 - yPos >= 1) canvas.fillRoundRect(xPos - 5, yPos, 10, py1 - yPos + 1, 2, INK);
         } else {
            int ink = (s == 0) ? INK : INK_MID;

            if (i > 0) DrawLine2(oldX, oldY, xPos, yPos, ink);
            canvas.fillCircle(xPos, yPos + 0, 3, ink);
            oldX = xPos;
            oldY = yPos;
         }
      }
   }
   for (int i = 0; i < n; i++) {
      int xPos = px0 + 6 + i * (px1 - px0 - 12) / (n - 1);

      DrawText(FONT_TINY, xPos, y + dy - 7, DayLetter(i), i == 0 ? INK : INK_MID, 1);
   }
}

/* ------------------------------------------------------------------ main */

/* Main function to show all the data to the e-paper */
void WeatherDisplay::Show()
{
   Serial.println("WeatherDisplay::Show");

   canvas.createCanvas(960, 540);
   canvas.fillCanvas(0);
   canvas.setTextSize(2);
   canvas.setTextColor(WHITE, BLACK);
   canvas.setTextDatum(TL_DATUM);

   DrawHead();

   if (!myData.weather.valid) {   // download failed: do not draw misleading zeros
      DrawNoData(M, TILE_Y0, TILE_X0 - M, 216);
      DrawIndoorTile(TILE_X1, TILE_Y0);
      canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
      delay(1000);
      return;
   }

   DrawHero(M, TILE_Y0, TILE_X0 - M, 216);
   DrawWindTile  (TILE_X0, TILE_Y0);
   DrawSunTile   (TILE_X1, TILE_Y0);
   DrawMoonTile  (TILE_X0, TILE_Y1);
   DrawIndoorTile(TILE_X1, TILE_Y1);

   DrawTile(M, HOURLY_Y, maxX - 2 * M, HOURLY_H);
   for (int i = 0; i < 8; i++) {
      DrawHourly(M + i * 112, HOURLY_Y, 112, HOURLY_H, myData.weather, i * 3);
   }

   int gw = (maxX - 2 * M - 3 * GAP) / 4;
   int gh = maxY - 10 - GRAPH_Y;
   Weather &w = myData.weather;

   // ranges come from the data: a mountain village needs other ones than the sea level
   DrawGraph(M + 0 * (gw + GAP), GRAPH_Y, gw, gh, "Temperatura", "", w.tempAxisMin, w.tempAxisMax, w.forecastMaxTemp, w.forecastMinTemp);
   DrawGraph(M + 1 * (gw + GAP), GRAPH_Y, gw, gh, "Lluvia", String(w.rainProbToday) + "% hoy", 0, w.maxRain, w.forecastRain, NULL, true);
   DrawGraph(M + 2 * (gw + GAP), GRAPH_Y, gw, gh, "Humedad", "", 0, 100, w.forecastHumidity);
   DrawGraph(M + 3 * (gw + GAP), GRAPH_Y, gw, gh, "Presión", "", w.pressureAxisMin, w.pressureAxisMax, w.forecastPressure);

   canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
   delay(1000);
}

/* Update only the indoor tile (partial refresh) */
void WeatherDisplay::ShowM5PaperInfo()
{
   Serial.println("WeatherDisplay::ShowM5PaperInfo");

   canvas.createCanvas(TILE_W, TILE_H);
   canvas.fillCanvas(0);
   DrawIndoorTile(0, 0);
   canvas.pushCanvas(TILE_X1, TILE_Y1, UPDATE_MODE_GC16);
   delay(1000);
}
