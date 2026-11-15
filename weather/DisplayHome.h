/**
  * @file DisplayHome.h
  *
  * Home Assistant page: six large tiles (caption, big value, two lines), same look as the weather pages.
  * Missing values are shown as "–"; nothing is invented.
  */
#pragma once
#include "Canvas.h"
#include "Data.h"
#include "Font.h"

namespace LayoutHome {

static const int M    = 32;
static const int GAP  = 12;
static const int TOP  = 24;
static const int INK      = 15;
static const int INK_MID  = 8;
static const int TILE_BG  = 1;

struct Tile {
   String caption, value, unit, line1, line2;
};

class HomeDisplay
{
protected:
   MyData &myData;
   int     maxX, maxY;

   const GlyphInfo *FindGlyph(const FontInfo &font, uint16_t cp)
   {
      for (int i = 0; i < font.count; i++) if (font.glyphs[i].cp == cp) return &font.glyphs[i];
      return NULL;
   }

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

   int TextWidth(const FontInfo &font, const String &text)
   {
      int w = 0;

      for (const char *p = text.c_str(); *p;) {
         const GlyphInfo *g = FindGlyph(font, NextCodePoint(p));

         if (g) w += g->adv;
      }
      return w;
   }

   void DrawText(const FontInfo &font, int x, int baseline, const String &text, int ink = INK, int align = 0, int clip = 100000)
   {
      if (align == 1) x -= TextWidth(font, text) / 2;
      if (align == 2) x -= TextWidth(font, text);
      for (const char *p = text.c_str(); *p;) {
         const GlyphInfo *g = FindGlyph(font, NextCodePoint(p));

         if (!g) continue;
         if (x + g->adv > clip) break;                 // never paint outside the tile
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

   /* 12,5 style number with the given decimals; "–" when the value is missing */
   static String Dec(float v, int digits)
   {
      char buf[24];

      if (isnan(v)) return String("–");
      snprintf(buf, sizeof(buf), "%.*f", digits, v);
      for (char *c = buf; *c; c++) if (*c == '.') *c = ',';
      return String(buf);
   }

   static String Whole(float v) { return isnan(v) ? String("–") : String((int) round(v)); }

   static String Duration(float minutes)
   {
      int m = (int) round(minutes);

      if (m >= 60) return String(m / 60) + " h " + String(m % 60) + " min";
      return String(m) + " min";
   }

   void DrawOneTile(int x, int y, int dx, int dy, const Tile &t)
   {
      canvas.fillRoundRect(x, y, dx, dy, 16, TILE_BG);
      DrawText(FONT_XCAP, x + 22, y + 40, t.caption, INK_MID);

      bool big = t.value.length() > 0 && !(t.value == "–");

      if (big) {
         DrawText(FONT_BIG, x + 18, y + 152, t.value);
         if (t.unit.length() > 0) DrawText(FONT_XBODY, x + 24 + TextWidth(FONT_BIG, t.value), y + 152, t.unit, INK_MID);
      } else {
         DrawText(FONT_XTITLE, x + 22, y + 152, "–", INK_MID);
      }
      DrawText(FONT_XBODY, x + 22, y + 196, t.line1, INK, 0, x + dx - 14);
      DrawText(FONT_XBODY, x + 22, y + 230, t.line2, INK_MID, 0, x + dx - 14);
   }

   /* ---- state texts ------------------------------------------------------------------ */
   static String EvText(const String &s)
   {
      if (s == "Available") return "Libre";
      if (s == "Preparing") return "Coche conectado";
      if (s == "Charging") return "Cargando";
      if (s == "SuspendedEV" || s == "SuspendedEVSE") return "Carga en pausa";
      if (s == "Finishing") return "Carga terminada";
      if (s == "Reserved") return "Reservado";
      if (s == "Unavailable") return "Sin conexión";
      if (s == "Faulted") return "Avería";
      return s.length() ? s : String("–");
   }

   static String VacText(const String &s)
   {
      if (s == "docked") return "En la base";
      if (s == "cleaning") return "Limpiando";
      if (s == "returning") return "Volviendo a la base";
      if (s == "idle") return "En reposo";
      if (s == "paused") return "En pausa";
      if (s == "error") return "Error";
      return s.length() ? s : String("–");
   }

   static String ModeText(const String &s)
   {
      if (s == "heat") return "Calefacción";
      if (s == "off") return "Apagado";
      if (s == "auto") return "Automático";
      if (s == "cool") return "Frío";
      return s.length() ? s : String("–");
   }

   void BuildTiles(Tile *t)
   {
      HAData &h = myData.ha;

      // thermostat
      t[0].caption = "Termostato";
      t[0].value   = isnan(h.thCur) ? String("–") : Dec(h.thCur, 1) + "°";
      t[0].line1   = isnan(h.thSet) ? String("") : "Consigna " + Dec(h.thSet, 1) + "°";
      t[0].line2   = h.thAction == "heating" ? String("Calentando") : ModeText(h.thMode);

      // Honda e
      bool charging = h.carCharge.length() && String(h.carCharge).indexOf("charging") >= 0;
      t[1].caption = "Honda e";
      t[1].value   = isnan(h.carBat) ? String("–") : Whole(h.carBat);
      t[1].unit    = isnan(h.carBat) ? String("") : "%";
      t[1].line1   = charging ? String("Cargando") : (h.carPlug == "plugged_in" ? String("Enchufado") : (h.carPlug == "unplugged" ? String("Sin enchufar") : String("–")));
      t[1].line2   = (charging && !isnan(h.carMinLeft) && h.carMinLeft > 0) ? "Lista en " + Duration(h.carMinLeft)
                   : (isnan(h.carRange) ? String("") : "Autonomía " + Whole(h.carRange) + " km");

      // charger
      bool evCharging = h.evStatus == "Charging" || (!isnan(h.evKw) && h.evKw > 0.1);
      t[2].caption = "Cargador";
      t[2].value   = evCharging ? Dec(h.evKw, 1) : String("");
      t[2].unit    = evCharging ? String("kW") : String("");
      t[2].line1   = EvText(h.evStatus);
      t[2].line2   = isnan(h.evSession) ? String("") : "Sesión " + Dec(h.evSession, 1) + " kWh";
      if (!evCharging) t[2].value = "–";

      // dehumidifier
      t[3].caption = "Cuarto de juegos";
      t[3].value   = isnan(h.dhHum) ? String("–") : Whole(h.dhHum);
      t[3].unit    = isnan(h.dhHum) ? String("") : "%";
      t[3].line1   = h.dhState == "on" ? String("Encendido") : (h.dhState == "off" ? String("Apagado") : String("–"));
      t[3].line2   = h.dhTankFull ? String("Depósito lleno") : (h.dhFilter ? String("Limpiar el filtro") : String(""));

      // vacuum
      t[4].caption = "Aspiradora";
      t[4].value   = isnan(h.vacBat) ? String("–") : Whole(h.vacBat);
      t[4].unit    = isnan(h.vacBat) ? String("") : "%";
      t[4].line1   = VacText(h.vacState);
      t[4].line2   = "";

      // litter box
      t[5].caption = "Arenero";
      t[5].value   = isnan(h.litCount) ? String("–") : Whole(h.litCount);
      t[5].line1   = isnan(h.litCount) ? String("") : (h.litCount == 1 ? String("visita hoy") : String("visitas hoy"));
      t[5].line2   = h.litState;
   }

public:
   HomeDisplay(MyData &md, int x = 960, int y = 540) : myData(md), maxX(x), maxY(y) {}

   void Show()
   {
      Serial.println("HomeDisplay::Show");
      canvas.deleteCanvas();
      canvas.createCanvas(960, 540);
      canvas.fillCanvas(0);

      if (!myData.ha.valid) {
         DrawText(FONT_XTITLE, maxX / 2, 250, "Sin datos de Home Assistant", INK, 1);
         DrawText(FONT_XBODY, maxX / 2, 296, "No se pudo leer la casa. Nuevo intento en el próximo refresco.", INK_MID, 1);
         canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
         delay(1000);
         return;
      }
      Tile tiles[6];
      int  tw = (maxX - 2 * M - 2 * 13) / 3;
      int  th = (maxY - TOP - 12 - GAP) / 2;

      BuildTiles(tiles);
      for (int i = 0; i < 6; i++) {
         DrawOneTile(M + (i % 3) * (tw + 13), TOP + (i / 3) * (th + GAP), tw, th, tiles[i]);
      }
      canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
      delay(1000);
   }
};

} // namespace LayoutHome
