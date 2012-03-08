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

#ifndef TAG_MODEL_HPP
#define TAG_MODEL_HPP

#include "treemodel.h"
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QPair>
#include <QList>
#include <QMimeData>
#include <QStringList>
#include <QList>

//our own mime type for ids
class TagModelItemMimeData : public QMimeData {
	Q_OBJECT
	public:
		TagModelItemMimeData();
		virtual QStringList formats() const;
		virtual bool hasFormat(const QString & mimeType ) const;
		virtual QVariant retrieveData ( const QString & mimeType, QVariant::Type type ) const;
		void addItem(int id);
	private:
		QList<int> mData;
};

class TagModel : public TreeModel {
	Q_OBJECT
	public:
		TagModel(const QSqlDatabase & db,
				QObject * parent = NULL);
		virtual ~TagModel();
		static int idColumn();
		static int countColumn();
		QSqlDatabase db() const;
		QList<QPair<int, QString> *> * classList() const;
		//this is how you associate a tag with a work
		void addWorkTagAssociation(int work_id, int tag_id);
		//delete an association between a tag and a work
		void removeWorkTagAssociation(int work_id, int tag_id);
		void removeWorkTagAssociation(int work_id, QModelIndex tag_index);
		//the below are here to allow draging of data
		Qt::ItemFlags flags(const QModelIndex &index) const;
		virtual QStringList mimeTypes() const;
		virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const;
		//idicates if this index is a tag or class..
		bool isTag(const QModelIndex index) const;
		//returns -1 on failure
		int find(std::string tag_name);
		int find(std::string tag_name, std::string class_name);
	signals:
		void classAdded(QPair<int, QString> classInfo);
		void tagAssociationMade(int work_id, int tag_id);
	public slots:
		void addTag(int classId, QString tagName);
		void addClassAndTag(QString className, QString tagName);
	private:
		QList<QPair<int, QString> *> * mClassList;
		QSqlDatabase mDb;
		void buildFromQuery();
		const static QString cQueryStr;
		QSqlQuery mQuery;
		QSqlQuery mAddTagQuery;
		void updateTagCount(int tag_id);
};

#endif
