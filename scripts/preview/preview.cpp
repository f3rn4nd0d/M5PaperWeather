#include "Data.h"
#include "Display.h"
int main(int argc, char **argv) {
   MyData d;
   if (argc > 1 && std::string(argv[1]) == "nodata") d.weather.valid = false;
   WeatherDisplay disp(d);
   disp.Show();
   FILE *f = fopen(argc > 2 ? argv[2] : "preview.pgm", "wb");
   fprintf(f, "P5\n960 540\n255\n");
   for (int i = 0; i < 960 * 540; i++) fputc(255 - SCREEN[i] * 17, f);
   fclose(f);
}
