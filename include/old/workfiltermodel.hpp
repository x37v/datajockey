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

#ifndef WORK_FILTER_MODEL_HPP
#define WORK_FILTER_MODEL_HPP

#include <QObject>
#include <QSortFilterProxyModel>
#include <string>
#include <set>

class WorkTableModel;
class ApplicationModel;

class WorkFilterModel : public QObject {
	Q_OBJECT
	public:
		WorkFilterModel(QObject * parent = NULL);
		virtual ~WorkFilterModel();
		//beforeFilter indicates if the filtering actually needs to be done
		//for instance, if the filter is based on tempo and them tempo hasn't
		//changed, then no work needs to be done
		virtual bool beforeFilter();
		virtual bool acceptsWork(int work_id) = 0;
		virtual std::string description() = 0;
		virtual std::string name() = 0;
	private:
		ApplicationModel * mApplicationModel;
};

Q_DECLARE_METATYPE(WorkFilterModel *)

//this is the one that actually does the filtering [based on a WorkFilterModel]
class WorkFilterModelProxy : public QSortFilterProxyModel {
	Q_OBJECT
	public:
		WorkFilterModelProxy(WorkTableModel * parent = NULL);
		WorkFilterModel * workFilter() const;
		virtual bool filterAcceptsRow(int sourceRow,
				const QModelIndex &sourceParent) const;
		virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	public slots:
		void setFilter(WorkFilterModel * model);
		//should we filter or let everything through?
		void filter(bool filter = true);
		void clearFilter();
	private:
		WorkFilterModel * mFilter;
		bool mFiltering;
};

#endif

