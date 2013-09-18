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

#ifndef WORK_DB_VIEW_HPP
#define WORK_DB_VIEW_HPP

#include <QWidget>
#include <QModelIndex>
#include <QItemSelection>
#include <QColor>
#include "db.hpp"

class QTableView;
class QPushButton;
class QAbstractItemModel;

//since we cannot style a QStyledItemDelegate we use this widget to specify style info

class SessionDisplayStyle : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QColor backgroundColor READ background_color WRITE background_color DESIGNABLE true)

  public:
    SessionDisplayStyle() : backgroundColor(Qt::red) { }

    QColor background_color() const { return backgroundColor; }
    void background_color(QColor color) { backgroundColor = color; }
  private:
    QColor backgroundColor;
};

class WorkDBView : public QWidget {
  Q_OBJECT
  public:
    WorkDBView(dj::model::DB * db, QAbstractItemModel * model, QWidget *parent = NULL);
    QTableView * tableView();
    void shouldWriteSettings(bool write);
    int last_selected() const { return mLastWork; }
  public slots:
    void select_work(int work_id);
    void select_last();
    void write_settings();
  protected slots:
    void read_settings();
    //void selectWork(const QModelIndex & index);
    void set_selection(const QItemSelection & selected);
    void set_selected_last(int work_id);
  signals:
    void work_selected(int work);
  private:
    dj::model::DB * mDB;
    QTableView * mTableView;
    bool mWriteSettings;
    int mLastWork;
};

#endif

