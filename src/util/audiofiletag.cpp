#include "audiofiletag.hpp"
#include <taglib/tag.h>
#include <taglib/fileref.h>

using namespace DataJockey::Util;

namespace {
	void fill_entry(const QString& tag_name, const TagLib::String& tag, QMap<QString, QVariant>& tag_data) {
		if (!tag.isEmpty())
			tag_data[tag_name] = QVariant(TStringToQString(tag.stripWhiteSpace()));
	}
}

void AudioFileTag::extract(const QString& path_to_file, QMap<QString, QVariant>& tag_data) throw(std::runtime_error) {
	TagLib::FileRef tag_file(path_to_file.toAscii());
	if (tag_file.isNull())
		throw std::runtime_error("AudioFileTag::extract cannot open file: " + path_to_file.toStdString());
	if (!tag_file.tag())
		throw std::runtime_error("AudioFileTag::extract cannot find file tags: " + path_to_file.toStdString());

	TagLib::Tag *tags = tag_file.tag();
	if (tags->isEmpty())
		throw std::runtime_error("AudioFileTag::extract empty tags: " + path_to_file.toStdString());

	fill_entry("title", tags->title(), tag_data);
	fill_entry("artist", tags->artist(), tag_data);
	fill_entry("album", tags->album(), tag_data);
	fill_entry("genre", tags->genre(), tag_data);
	fill_entry("comment", tags->comment(), tag_data);

	if(tags->year() != 0)
		tag_data["year"] = QVariant(tags->year());
	if(tags->track() != 0)
		tag_data["track"] = QVariant(tags->track());
}
