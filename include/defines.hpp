#ifndef DATAJOCKEY_DEFINES_H
#define DATAJOCKEY_DEFINES_H

#include <string>
#include <sstream>
#include <QString>
#include <QPair>

#define DO_STRINGIFY(X) #X
#define STRINGIFY(X) DO_STRINGIFY(X)
#define INAUDIBLE_VOLUME 0.001f

namespace dj {
   //this is the value we use to scale from an int to a double, this
   //represents 'one' as a double in int terms.
   extern const unsigned int one_scale;
   
   const QString version_string = STRINGIFY(DJ_VERSION);

   typedef QPair<QString, int> OscNetAddr;

   template <typename T>
   std::string to_string(T v) {
      std::ostringstream result;
      result << v;
      return result.str();
   }

   template <typename T>
      T clamp(T val, T bottom, T top) {
         if (val < bottom)
            return bottom;
         if (val > top)
            return top;
         return val;
      }

   const int volume_slider_height = 200;
}


#define DJ_FILEANDLINE std::string(std::string(__FILE__) + " " + dj::to_string(__LINE__) + " ")

#endif
