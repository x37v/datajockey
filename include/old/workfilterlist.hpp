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

#ifndef WORK_FILTER_LIST_HPP
#define WORK_FILTER_LIST_HPP

#include <QObject>
#include <QList>

class WorkFilterModel;

class WorkFilterList : public QObject {
	Q_OBJECT
	public:
		WorkFilterList(QObject * parent = NULL);
		WorkFilterModel * selected() const;
		unsigned int selectedIndex() const;
		QList<WorkFilterModel *> list() const;
		WorkFilterModel * at(unsigned int i) const;
		unsigned int size() const;
	signals:
		void filterAdded(unsigned int list_size);
		void filterRemoved(unsigned int index);
		void selectionChanged(unsigned int new_index, unsigned int old_index);
		void selectionChanged(WorkFilterModel * selected);
	public slots:
		void addFilter(WorkFilterModel * filter);
		void removeFilter(unsigned int index);
		void removeFilter(WorkFilterModel * filter);
		void selectFilter(unsigned int index);
		void selectFilter(WorkFilterModel * filter);
	private:
		int mSelected;
		QList<WorkFilterModel *> mModelList;
};

#endif

