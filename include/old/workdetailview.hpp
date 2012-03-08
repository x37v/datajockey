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

#ifndef WORK_DETAIL_VIEW_HPP
#define WORK_DETAIL_VIEW_HPP

#include <QWidget>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

class QLineEdit;
class QGridLayout;
class TagView;
class WorkTagModelFilter;
class TagModel;
class QPushButton;

class WorkDetailView : public QWidget {
	Q_OBJECT
	public:
		WorkDetailView(
				TagModel * tagModel,
				const QSqlDatabase & db,
				QWidget * parent = NULL);
		virtual void keyPressEvent ( QKeyEvent * event );
	signals:
		void previewing(bool previewing);
	public slots:
		void setWork(int work_id);
		void clear();
		void expandTags(bool expand = true);
		void setPreviewing(bool down);
	protected slots:
		void setPreviewingInternal(bool down);
	private:
		static const QString cWorkQuery;
		QGridLayout * mLayout;
		QLineEdit * mArtist;
		QLineEdit * mTitle;
		TagView * mTagView;
		QPushButton * mPreviewButton;

		WorkTagModelFilter * mTagModel;
		bool mDisplayingWork;

		QSqlQuery mWorkQuery;
};

#endif

