#include "audiofiletag.h"
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <QFile>

namespace {
	void fill_entry(const QString& tag_name, const TagLib::String& tag, QHash<QString, QVariant>& tag_data) {
		if (!tag.isEmpty())
			tag_data[tag_name] = QVariant(TStringToQString(tag.stripWhiteSpace()));
	}
}

void audiofiletag::extract(const QString& path_to_file, QHash<QString, QVariant>& tag_data) throw(std::runtime_error) {
	TagLib::FileRef tag_file(QFile::encodeName(path_to_file));
	if (tag_file.isNull())
		throw std::runtime_error("audiofile_tag::extract cannot open file: " + path_to_file.toStdString());
	if (!tag_file.tag())
		throw std::runtime_error("audiofile_tag::extract cannot find file tags: " + path_to_file.toStdString());

	TagLib::Tag *tags = tag_file.tag();
	if (tags->isEmpty())
		throw std::runtime_error("audiofile_tag::extract empty tags: " + path_to_file.toStdString());

	fill_entry("name", tags->title(), tag_data);
	fill_entry("artist", tags->artist(), tag_data);
	fill_entry("album", tags->album(), tag_data);
	fill_entry("genre", tags->genre(), tag_data);
	fill_entry("comment", tags->comment(), tag_data);

	if(tags->year() != 0)
		tag_data["year"] = QVariant(tags->year());
	if(tags->track() != 0)
		tag_data["track"] = QVariant(tags->track());
}

