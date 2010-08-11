#ifndef DATAJOCKEY_TYPES_HPP
#define DATAJOCKEY_TYPES_HPP

#include <yamlcpp/yaml.hpp>

namespace DataJockey {
	//this is the type of data which we use for storing and restoring a command from file
	typedef yaml::map CommandIOData;
}

#endif
