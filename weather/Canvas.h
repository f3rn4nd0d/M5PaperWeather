/**
  * @file Canvas.h
  *
  * The one canvas of the e-paper, shared by all the page layouts.
  */
#pragma once
#include <M5EPD.h>

M5EPD_Canvas canvas(&M5.EPD); // Main canvas of the e-paper
