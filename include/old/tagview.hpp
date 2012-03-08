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

#ifndef TAG_VIEW_HPP
#define TAG_VIEW_HPP

#include <QTreeView>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "treemodel.h"
#include <QList>

class TreeModel;
class QAbstractItemModel;

class TagView : public QTreeView {
	Q_OBJECT
	public:
		TagView(QAbstractItemModel * model,
				QWidget * parent = NULL);
		//give a list of the ids of the selected tags
		QList<int> selectedTagIds() const;
	signals:
		void tagSelectionChanged(QList<int>);
	protected slots:
		void updateTagSelection();
	private:
		QAbstractItemModel * mModel;
};

#endif
