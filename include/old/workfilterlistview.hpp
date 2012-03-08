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

#ifndef WORK_FILTER_LIST_VIEW_HPP
#define WORK_FILTER_LIST_VIEW_HPP

#include <QWidget>
#include <QItemSelection>

class WorkFilterList;
class QTableView;
class QTextEdit;
class QStandardItemModel;
class QStandardItem;

class WorkFilterListView : public QWidget {
	Q_OBJECT
	public:
		WorkFilterListView(WorkFilterList * sourceList, QWidget * parent = NULL);
	public slots:
		void addFilter(unsigned int list_size);
		void removeFilter(unsigned int index);
		//void setSelection(unsigned int index);
		void updateSelection(unsigned int index, unsigned int old_index);
	signals:
		void selectionChanged(unsigned int selection);
	protected slots:
		void listDataChanged(QStandardItem * item);
		void setSelection( const QItemSelection & selected);
	private:
		WorkFilterList * mFilterList;
		QTextEdit * mDetails;
		QTableView * mSelectionTable;
		QStandardItemModel * mModel;
};

#endif

