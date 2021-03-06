#ifndef DATAJOCKEY_AUDIOFILE_TAG_HPP
#define DATAJOCKEY_AUDIOFILE_TAG_HPP

#include <stdexcept>
#include <QString>
#include <QHash>
#include <QVariant>

namespace audiofiletag {
  void extract(const QString& path_to_file, QHash<QString, QVariant>& tag_data) throw(std::runtime_error);
}

#endif
