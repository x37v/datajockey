#ifndef DATAJOCKEY_DEFINES_H
#define DATAJOCKEY_DEFINES_H

#include <string>
#include <sstream>

namespace dj {
   //this is the value we use to scale from an int to a double, this
   //represents 'one' as a double in int terms.
   extern const unsigned int one_scale;

   template <typename T>
   std::string to_string(T v) {
      std::ostringstream result;
      result << v;
      return result.str();
   }
}


#define DJ_FILEANDLINE std::string(std::string(__FILE__) + " " + dj::to_string(__LINE__) + " ")

#endif
