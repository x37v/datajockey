#ifndef DATAJOCKEY_AUDIOFILE_TAG_HPP
#define DATAJOCKEY_AUDIOFILE_TAG_HPP

#include <stdexcept>
#include <QString>
#include <QMap>
#include <QVariant>

namespace DataJockey {
	namespace Util {
		namespace AudioFileTag {
			void extract(const QString& path_to_file, QMap<QString, QVariant>& tag_data) throw(std::runtime_error);
		}
	}
}

#endif
