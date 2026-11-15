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
#include "Canvas.h"
#include "Data.h"
#include "Font.h"
#include "DisplayA.h"
#include "DisplayB.h"
#include "DisplayHome.h"

/* The pages of the M5Paper. The wheel moves between them; the Home Assistant page only exists with a token. */
enum PageId { PAGE_WEATHER_HEADER = 0, PAGE_WEATHER_CLEAN = 1, PAGE_HOME = 2 };

class Pages
{
protected:
   MyData                    &myData;
   LayoutA::WeatherDisplay    a;
   LayoutB::WeatherDisplay    b;
   LayoutHome::HomeDisplay    home;

public:
   Pages(MyData &md) : myData(md), a(md), b(md), home(md) {}

   /* number of pages available */
   int Count() { return HaConfigured() ? 3 : 2; }

   /* keep a stored page number inside the available range */
   int Clamp(int page) { return (page < 0 || page >= Count()) ? 0 : page; }

   void Show(int page)
   {
      switch (page) {
         case PAGE_WEATHER_CLEAN: b.Show();    break;
         case PAGE_HOME:          home.Show(); break;
         default:                 a.Show();    break;
      }
   }

   /* partial refresh of the indoor tile (REFRESH_PARTLY mode only; the Home page has no such tile) */
   void ShowM5PaperInfo(int page)
   {
      if (page == PAGE_WEATHER_CLEAN) b.ShowM5PaperInfo();
      else if (page == PAGE_HOME)     home.Show();
      else                            a.ShowM5PaperInfo();
   }
};
