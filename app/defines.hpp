#ifndef DATAJOCKEY_DEFINES_H
#define DATAJOCKEY_DEFINES_H

#include <string>
#include <sstream>
#include <QString>
#include <QStringList>
#include <QPair>
#include <yaml-cpp/yaml.h>

#define DO_STRINGIFY(X) #X
#define STRINGIFY(X) DO_STRINGIFY(X)
#define INAUDIBLE_VOLUME 0.001f

namespace dj {
   //this is the value we use to scale from an int to a double, this
   //represents 'one' as a double in int terms.
   extern const unsigned int one_scale;
   extern const double pi;
   
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

   template<typename Type>
     Type linear_interp(Type v0, Type v1, double dist){
       return v0 + (v1 - v0) * dist;
     }

   const int volume_slider_height = 50;

   const QStringList audio_file_extensions = (QStringList() << "flac" << "mp3" << "ogg");

   enum loop_and_jump_type_t { JUMP_BEAT, JUMP_FRAME, LOOP_BEAT, LOOP_FRAME };
}

//convert to and from QString
namespace YAML {
  template<>
    struct convert<QString> {
      static Node encode(const QString& rhs) {
        Node node(rhs.toStdString());
        return node;
      }

      static bool decode(const Node& node, QString& rhs) {
        rhs = QString::fromStdString(node.as<std::string>());
        return true;
      }
    };
}


#define DJ_FILEANDLINE std::string(std::string(__FILE__) + " " + dj::to_string(__LINE__) + " ")

#endif
