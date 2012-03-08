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

#ifndef TAG_EDITOR_HPP
#define TAG_EDITOR_HPP

#include <QWidget>
#include <QSpinBox>

class TagModel;
class TagView;
class QPushButton;
class QLineEdit;
class QComboBox;

class TagEditor : public QWidget {
	Q_OBJECT
	public:
		TagEditor(TagModel * model, QWidget * parent = NULL);
	signals:
		//add a tag to an existing class
		void tagAdded(int classIndex, QString tagName);
		//create a new class with this tag
		void tagAdded(QString className, QString tagName);
		void tagSelectionChanged(QList<int> tags_selected);
	protected slots:
		void addTagButtonPushed();
		void addToClassList(QPair<int, QString> item);
	private:
		TagView * mTagView;
		QPushButton * mAddTagBtn;
		QComboBox * mTagClassSelect;
		QLineEdit * mTagNameInput;
};

#endif

