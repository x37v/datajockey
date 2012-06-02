/*
 *		Copyright (c) 2008 Alex Norman.  All rights reserved.
 *		http://www.x37v.info/datajockey
 *
 *		This file is part of Data Jockey.
 *		
 *		Data Jockey is free software: you can redistribute it and/or modify it
 *		under the terms of the GNU General Public License as published by the
 *		Free Software Foundation, either version 3 of the License, or (at your
 *		option) any later version.
 *		
 *		Data Jockey is distributed in the hope that it will be useful, but
 *		WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *		Public License for more details.
 *		
 *		You should have received a copy of the GNU General Public License along
 *		with Data Jockey.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEFAULT_WORK_FILTERS_HPP
#define DEFAULT_WORK_FILTERS_HPP

#include "workfiltermodel.hpp"
#include <set>
#include <QList>
#include <QSqlQuery>

namespace dj {
   namespace audio {
      class AudioModel;
   }
}

class TagSelectionFilter : public WorkFilterModel {
	Q_OBJECT
	public:
		TagSelectionFilter(QObject * parent = NULL);
		virtual bool beforeFilter();
		virtual bool acceptsWork(int work_id);
		virtual std::string description();
		virtual std::string name();
	public slots:
		void addTag(int tag_id);
		void clearTags();
		void setTags(QList<int> tags);
	private:
		QSqlQuery mQuery;
		QList<int> mSelectedTags;
		std::set<int> mSelectedWorks;
};

class TempoRangeFilter : public WorkFilterModel {
	Q_OBJECT
	public:
		TempoRangeFilter(QObject * parent = NULL);
		virtual bool beforeFilter();
		virtual bool acceptsWork(int work_id);
		virtual std::string description();
		virtual std::string name();
	public slots:
		void setRange(float below, float above);
	private:
		QSqlQuery mQuery;
		std::set<int> mSelectedWorks;
		float mBelow;
		float mAbove;
		float mPrevTempo;
      dj::audio::AudioModel * mAudioModel;
};


#endif

