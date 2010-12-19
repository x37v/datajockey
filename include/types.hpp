#ifndef DATAJOCKEY_TYPES_HPP
#define DATAJOCKEY_TYPES_HPP

//#include <yamlcpp/yaml.hpp>

#include <map>
#include <boost/variant/variant.hpp>

namespace DataJockey {
	//this is the type of data which we use for storing and restoring a command from file
   typedef std::map<std::string, boost::variant<std::string, int, double> > CommandIOData;
   //typedef yaml::map CommandIOData;
}

#endif
