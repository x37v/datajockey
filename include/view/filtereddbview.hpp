/*
 *		Copyright (c) 2012 Alex Norman.  All rights reserved.
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

#ifndef FILTERED_DB_VIEW_HPP
#define FILTERED_DB_VIEW_HPP

#include <QWidget>

class WorkDBView;
class QAbstractItemModel;
class QTextEdit;

class FilteredDBView : public QWidget {
  Q_OBJECT
  public:
    FilteredDBView(QAbstractItemModel * model, QWidget *parent = NULL);
  public slots:
    void select_work(int work_id);
    void set_filter_expression(QString expression);
    void filter_expression_error(QString expression);
  protected slots:
    void submit_pressed();
  signals:
    void work_selected(int work);
    void filter_expression_changed(QString expression);
  private:
    WorkDBView * mDBView;
    QTextEdit * mFilterEditor;
};

#endif
