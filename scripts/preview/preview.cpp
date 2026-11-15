#include "Data.h"
#include "Display.h"
// preview <page 0|1|2> <out.pgm> [nodata|noha]
int main(int argc, char **argv) {
   MyData d;
   int page = argc > 1 ? atoi(argv[1]) : 0;
   std::string mode = argc > 3 ? argv[3] : "";
   if (mode == "nodata") d.weather.valid = false;
   if (mode == "noha") d.ha.valid = false;
   Pages pages(d);
   pages.Show(page);
   FILE *f = fopen(argc > 2 ? argv[2] : "preview.pgm", "wb");
   fprintf(f, "P5\n960 540\n255\n");
   for (int i = 0; i < 960 * 540; i++) fputc(255 - SCREEN[i] * 17, f);
   fclose(f);
}
