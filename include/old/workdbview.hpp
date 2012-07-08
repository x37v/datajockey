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

#ifndef WORK_DB_VIEW_HPP
#define WORK_DB_VIEW_HPP

#include <QWidget>
#include <QModelIndex>
#include <QItemSelection>

class QTableView;
class QPushButton;
class QAbstractItemModel;

class WorkDBView : public QWidget {
	Q_OBJECT
	public:
		WorkDBView(QAbstractItemModel * model, QWidget *parent = NULL);
		QTableView * tableView();
		QPushButton * applyFilterButton();
		QPushButton * removeFilterButton();
	public slots:
		void selectWork(int work_id);
		void showFilterButtons(bool show = true);
      void write_settings();
	protected slots:
      void read_settings();
		//void selectWork(const QModelIndex & index);
		void setSelection( const QItemSelection & selected);
		void setFiltered();
		void setUnFiltered();
	signals:
		void workSelected(int work);
		void applyFilterPushed();
		void removeFilterPushed();
      void filter_state_changed(bool);

   private:
		QTableView * mTableView;
		QPushButton * mApplyFilterButton;
		QPushButton * mRemoveFilterButton;
};

#endif

