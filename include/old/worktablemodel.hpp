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

#ifndef WORK_TABLE_MODEL_HPP
#define WORK_TABLE_MODEL_HPP

//QSortFilterProxyModel

#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QString>

class QSqlDatabase;
class QSqlRecord;
#define TIME_COLUMN 5

class WorkTableModel : public QSqlQueryModel {
	Q_OBJECT
	public:
		WorkTableModel(
				const QSqlDatabase & db,
				QObject * parent = NULL
				);
		virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

		const static unsigned int idColumn;
		const static unsigned int artistColumn;
		const static unsigned int albumColumn;
		const static unsigned int titleColumn;
		const static unsigned int trackColumn;

		//returns a work id, or -1 on failure
		int findWorkByPath(std::string path);
		//returns true on success
		bool rateWork(int work_id, float rating);
	private:
		static void init(const QSqlDatabase & db);
		static bool cInited;
		static QString cQuery;
		const QSqlDatabase * mDB;
};

#endif

