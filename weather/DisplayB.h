// Layout B: no header, larger top half (design-v4-sin-cabecera).
#pragma once
#include "Canvas.h"
#include "Data.h"
#include "Icons.h"
#include "Font.h"

namespace LayoutB {

/*
 * Layout (960 x 540), inspired by Apple's weather: one big number, rounded tiles, no frames,
 * no section titles, one typeface (Inter, see scripts/make_font.py) and four ink levels.
 *
 *   (no header: the whole top half belongs to the weather)
 *   hero    : date / temperature / icon        | wind  sun
 *                                              | moon  indoor
 *   hourly  : 8 slots, every 3 hours
 *   graphs  : temperature, rain, humidity, pressure (8 days)
 */
static const int M        = 32;    //!< Outer margin
static const int GAP      = 12;    //!< Gap between tiles
static const int TOP_Y    = 24;    //!< Top of the weather block (there is no header)
static const int TOP_H    = 258;   //!< Height of the hero + tiles block
static const int TILE_W   = 240;   //!< Width of a tile
static const int TILE_H   = (TOP_H - GAP) / 2;
static const int TILE_X0  = 436;   //!< Left column of the tiles
static const int TILE_X1  = TILE_X0 + TILE_W + GAP;
static const int TILE_Y0  = TOP_Y;
static const int TILE_Y1  = TILE_Y0 + TILE_H + GAP;
static const int HOURLY_Y = TOP_Y + TOP_H + 8;
static const int HOURLY_H = 122;
static const int GRAPH_Y  = 420;

static const int INK      = 15;    //!< Black
static const int INK_MID  = 8;     //!< Secondary text
static const int INK_SOFT = 5;     //!< Hairlines and the second graph line
static const int TILE_BG  = 1;     //!< Very light grey behind the tiles


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
   void   DrawLine2(int x0, int y0, int x1, int y1, int ink);
   void   DrawTile(int x, int y, int dx, int dy);
   void   DrawIcon(int x, int y, const uint16_t *icon, int dx = 64, int dy = 64, bool highContrast = false);
   void   DrawWeatherIcon(int x, int y, const String &icon, int scaleNum = 1, int scaleDen = 1);

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

/* The 64x64 icon data for an openweathermap style code (01d, 10n ...) */
static const uint16_t *WeatherIconData(const String &icon)
{
        if (icon == "01d") return (const uint16_t *) image_data_01d;
   else if (icon == "01n") return (const uint16_t *) image_data_03n;
   else if (icon == "02d") return (const uint16_t *) image_data_02d;
   else if (icon == "02n") return (const uint16_t *) image_data_02n;
   else if (icon == "03d") return (const uint16_t *) image_data_03d;
   else if (icon == "03n") return (const uint16_t *) image_data_03n;
   else if (icon == "04d") return (const uint16_t *) image_data_04d;
   else if (icon == "04n") return (const uint16_t *) image_data_03n;
   else if (icon == "09d") return (const uint16_t *) image_data_09d;
   else if (icon == "09n") return (const uint16_t *) image_data_09n;
   else if (icon == "10d") return (const uint16_t *) image_data_10d;
   else if (icon == "10n") return (const uint16_t *) image_data_03n;
   else if (icon == "11d") return (const uint16_t *) image_data_11d;
   else if (icon == "11n") return (const uint16_t *) image_data_11n;
   else if (icon == "13d") return (const uint16_t *) image_data_13d;
   else if (icon == "13n") return (const uint16_t *) image_data_13n;
   else if (icon == "50d") return (const uint16_t *) image_data_50d;
   else if (icon == "50n") return (const uint16_t *) image_data_50n;
   return (const uint16_t *) image_data_unknown;
}

/* Draw a weather icon; scaleNum / scaleDen enlarge it with bilinear smoothing (3/2 = 96 px) */
void WeatherDisplay::DrawWeatherIcon(int x, int y, const String &icon, int scaleNum, int scaleDen)
{
   const uint16_t *data = WeatherIconData(icon);
   int             size = 64 * scaleNum / scaleDen;

   for (int yi = 0; yi < size; yi++) {
      for (int xi = 0; xi < size; xi++) {
         float sx = (xi + 0.5f) * scaleDen / scaleNum - 0.5f;
         float sy = (yi + 0.5f) * scaleDen / scaleNum - 0.5f;
         int   x0 = (int) floor(sx), y0 = (int) floor(sy);
         float fx = sx - x0, fy = sy - y0;
         float v  = 0;

         for (int k = 0; k < 4; k++) {
            int   px = x0 + (k & 1), py = y0 + (k >> 1);
            float wt = ((k & 1) ? fx : 1 - fx) * ((k >> 1) ? fy : 1 - fy);

            if (px >= 0 && py >= 0 && px < 64 && py < 64 && 15 - (data[py * 64 + px] / 4096) > 0) v += 15 * wt;
         }
         if ((int) round(v) > 0) canvas.drawPixel(x + xi, y + yi, (int) round(v));
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

/* ------------------------------------------------------------------ hero */

/* Date, current temperature, icon, min / max and the short description (all centred) */
void WeatherDisplay::DrawHero(int x, int y, int dx, int dy)
{
   Weather &w    = myData.weather;
   String   temp = String((int) round(w.currentTemp)) + "°";
   String   hi   = "Máx " + String((int) round(w.forecastMaxTemp[0])) + "°";
   String   lo   = "Mín " + String((int) round(w.forecastMinTemp[0])) + "°";
   String   desc = w.hourlyMain[0] + "  ·  sensación " + String((int) round(w.currentThermalComfort)) + "°";
   int      cx   = x + dx / 2;                        // centred between the margin and the tiles

   DrawText(FONT_XBODY, cx, y + 30, LongDate(), INK_MID, 1);

   // big number + (icon over max / min) as one block
   int iconS = 96;                                    // the 64 px icon, enlarged 3/2
   int numW  = TextWidth(FONT_HERO, temp);
   int sideW = max(iconS, max(TextWidth(FONT_XBODY, hi), TextWidth(FONT_XBODY, lo)));
   int gap   = 22;
   int left  = cx - (numW + gap + sideW) / 2;
   int sideX = left + numW + gap;

   DrawText(FONT_HERO, left, y + 196, temp);
   DrawWeatherIcon(sideX + (sideW - iconS) / 2, y + 46, w.hourlyIcon[0], 3, 2);
   DrawText(FONT_XBODY, sideX + (sideW - TextWidth(FONT_XBODY, hi)) / 2, y + 166, hi);
   DrawText(FONT_XBODY, sideX + (sideW - TextWidth(FONT_XBODY, lo)) / 2, y + 198, lo, INK_MID);

   DrawText(FONT_XBODY, cx, y + 246, desc, INK_MID, 1);
}

/* Message shown instead of the weather when the download failed */
void WeatherDisplay::DrawNoData(int x, int y, int dx, int dy)
{
   int cx = x + dx / 2;

   DrawText(FONT_XBODY, cx, y + 30, LongDate(), INK_MID, 1);
   DrawText(FONT_XTITLE, cx, y + 120, "Sin datos del tiempo", INK, 1);
   DrawText(FONT_XBODY, cx, y + 170, "No se pudo descargar la previsión.", INK_MID, 1);
   DrawText(FONT_XBODY, cx, y + 206, "Nuevo intento en " + String(RETRY_MINUTES) + " minutos.", INK_MID, 1);
}

/* ------------------------------------------------------------------ tiles */

static const char *CompassName(float deg)
{
   static const char *names[8] = {"Norte", "Noreste", "Este", "Sureste", "Sur", "Suroeste", "Oeste", "Noroeste"};

   return names[((int) round(deg / 45.0)) % 8];
}

/* Wind: a compass with the arrow pointing where the wind blows to, speed and origin.
 * Every tile measures its content and centres it, so nothing hugs an edge. */
void WeatherDisplay::DrawWindTile(int x, int y)
{
   float  angle = myData.weather.winddir;
   String speed = String((int) round(myData.weather.windspeed));
   String name  = CompassName(angle);
   int    r     = 40;
   int    textW = max(TextWidth(FONT_XTITLE, speed) + 6 + TextWidth(FONT_XCAP, "km/h"), TextWidth(FONT_XBODY, name));
   int    gap   = 16;
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

      canvas.drawLine(cx + (r - 9) * sin(t), cy - (r - 9) * cos(t), cx + (r - 1) * sin(t), cy - (r - 1) * cos(t), i == 0 ? INK : INK_SOFT);
   }
   // arrow: from the origin of the wind through the centre, head on the far side
   DrawLine2(cx + ux * (r - 13), cy + uy * (r - 13), cx - ux * 9, cy - uy * 9, INK);
   double hx = cx - ux * 20, hy = cy - uy * 20;      // tip
   double bx = cx - ux * 5,  by = cy - uy * 5;       // base centre
   canvas.fillTriangle(hx, hy, bx - uy * 8, by + ux * 8, bx + uy * 8, by - ux * 8, INK);

   DrawText(FONT_XTITLE, tx, cy - 1, speed);
   DrawText(FONT_XCAP, tx + TextWidth(FONT_XTITLE, speed) + 6, cy - 1, "km/h", INK_MID);
   DrawText(FONT_XBODY, tx, cy + 32, name, INK_MID);
}

/* Sun: sunrise and sunset, the block (arrow + time) centred in the tile */
void WeatherDisplay::DrawSunTile(int x, int y)
{
   String rise  = getHourMinString(myData.weather.sunrise);
   String set   = getHourMinString(myData.weather.sunset);
   int    timeW = max(TextWidth(FONT_XTITLE, rise), TextWidth(FONT_XTITLE, set));
   int    left  = x + (TILE_W - (20 + 22 + timeW)) / 2;

   DrawTile(x, y, TILE_W, TILE_H);
   DrawTriangle(left, y + 23, 20, true, INK);
   DrawText(FONT_XTITLE, left + 42, y + 56, rise);
   DrawTriangle(left, y + 72, 20, false, INK_MID);
   DrawText(FONT_XTITLE, left + 42, y + 105, set, INK_MID);
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
   int    r     = 30;
   int    rowW  = (9 + 5 + TextWidth(FONT_CAP, rise)) + 16 + (9 + 5 + TextWidth(FONT_CAP, set));
   int    textW = max(TextWidth(FONT_XBODY, name), rowW);
   int    gap   = 16;
   int    left  = x + (TILE_W - (2 * r + gap + textW)) / 2;
   int    tx    = left + 2 * r + gap;
   int    x2    = tx + (9 + 5 + TextWidth(FONT_CAP, rise)) + 16;

   DrawTile(x, y, TILE_W, TILE_H);
   DrawMoonDisc(left + r, y + TILE_H / 2, r, phase);
   DrawText(FONT_XBODY, tx, y + 40, name);
   DrawText(FONT_XCAP, tx, y + 68, String(illum) + "%", INK_MID);
   DrawTriangle(tx, y + 88, 9, true, INK);
   DrawText(FONT_CAP, tx + 14, y + 99, rise);
   DrawTriangle(x2, y + 88, 9, false, INK_MID);
   DrawText(FONT_CAP, x2 + 14, y + 99, set, INK_MID);
}

/* Indoor: SHT30 temperature and humidity, a house and a drop as the only "titles".
 * Each column (symbol over value) is centred on itself and the pair is centred in the tile. */
void WeatherDisplay::DrawIndoorTile(int x, int y)
{
   String t   = String(myData.sht30Temperatur);
   String h   = String(myData.sht30Humidity);
   int    w1  = TextWidth(FONT_XTITLE, t) + 3 + TextWidth(FONT_XCAP, "°C");
   int    w2  = TextWidth(FONT_XTITLE, h) + 3 + TextWidth(FONT_XCAP, "%");
   int    gap = 32;
   int    x1  = x + (TILE_W - (w1 + gap + w2)) / 2;   // left of the temperature column
   int    x2  = x1 + w1 + gap;                        // left of the humidity column
   int    c1  = x1 + w1 / 2, c2 = x2 + w2 / 2;
   int    base = y + 103;

   DrawTile(x, y, TILE_W, TILE_H);
   // house
   canvas.fillTriangle(c1 - 15, y + 36, c1 + 15, y + 36, c1, y + 19, INK_MID);
   canvas.fillRect(c1 - 10, y + 36, 20, 13, INK_MID);
   // drop
   canvas.fillCircle(c2, y + 38, 9, INK_MID);
   canvas.fillTriangle(c2 - 9, y + 35, c2 + 9, y + 35, c2, y + 17, INK_MID);

   DrawText(FONT_XTITLE, x1, base, t);
   DrawText(FONT_XCAP, x1 + TextWidth(FONT_XTITLE, t) + 3, base, "°C", INK_MID);
   DrawText(FONT_XTITLE, x2, base, h);
   DrawText(FONT_XCAP, x2 + TextWidth(FONT_XTITLE, h) + 3, base, "%", INK_MID);
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
   DrawText(FONT_CAP, cx, y + 25, label, index == 0 ? INK : INK_MID, 1);
   DrawWeatherIcon(cx - 32, y + 31, weather.hourlyIcon[index]);
   DrawText(FONT_BODY, cx, y + 116, String(temp) + "°", INK, 1);
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

   canvas.deleteCanvas();
   canvas.createCanvas(960, 540);
   canvas.fillCanvas(0);
   canvas.setTextSize(2);
   canvas.setTextColor(WHITE, BLACK);
   canvas.setTextDatum(TL_DATUM);

   if (!myData.weather.valid) {   // download failed: do not draw misleading zeros
      DrawNoData(M, TILE_Y0, TILE_X0 - M, TOP_H);
      DrawIndoorTile(TILE_X1, TILE_Y0);
      canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
      delay(1000);
      return;
   }

   DrawHero(M, TILE_Y0, TILE_X0 - M, TOP_H);
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

   canvas.deleteCanvas();
   canvas.createCanvas(TILE_W, TILE_H);
   canvas.fillCanvas(0);
   DrawIndoorTile(0, 0);
   canvas.pushCanvas(TILE_X1, TILE_Y1, UPDATE_MODE_GC16);
   delay(1000);
}

} // namespace LayoutB
