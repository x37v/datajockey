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

#ifndef WORK_TAG_MODEL_FILTER_HPP
#define WORK_TAG_MODEL_FILTER_HPP

#include <QSortFilterProxyModel>
#include <QSqlQuery>
#include <QModelIndex>
#include <set>

class TagModel;

class WorkTagModelFilter : public QSortFilterProxyModel {
	Q_OBJECT
	public:
		WorkTagModelFilter(TagModel * model);
		int work();
		TagModel * sourceTagModel();
		virtual bool filterAcceptsRow(int sourceRow,
				const QModelIndex &sourceParent) const;
		//the below are here to allow for droping of drag data
		Qt::DropActions supportedDropActions() const;
		Qt::ItemFlags flags(const QModelIndex &index) const;
		virtual bool dropMimeData( const QMimeData * data, 
				Qt::DropAction action, int row, int column, 
				const QModelIndex & parent );
	signals:
		void workChanged(int);
	public slots:
		void setWork(int work);
		void clear();
		void refilter();
	private:
		QSqlQuery mWorkTagsQuery;
		int mWork;
		std::set<int> mTagClassIds;
		std::set<int> mTagIds;
		TagModel * mModel;
};

#endif

