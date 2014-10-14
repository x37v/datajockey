#include "defines.hpp"
#include <cmath>

namespace dj {
   const unsigned int one_scale = 1000;
   const double pi = 4.0 * atan(1.0);
   double db2amp(double db) { return pow(10, db / 20.0); }
   double amp2db(double amp) { return 20 * log10(amp); }
}

