#include "defines.hpp"
#include <cmath>

namespace dj {
   const unsigned int one_scale = 1000;
   const double pi = 4.0 * atan(1.0);
   double db2amp(double db) { return pow(10, db / 20.0); }
   double amp2db(double amp) { return 20 * log10(amp); }

   //convert between our integer based 'one_scale' and double
   const double done_scale = static_cast<double>(dj::one_scale);
   int to_int(double v) { return static_cast<int>(v * done_scale); }
   double to_double(int v) { return static_cast<double>(v) / done_scale; }
}

