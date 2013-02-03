#ifndef WORKS_TAB_VIEW_HPP
#define WORKS_TAB_VIEW_HPP

#include <QTabWidget>
#include <QList>

#include "workdbview.hpp"
#include "filtereddbview.hpp"
#include "workfiltermodelcollection.hpp"

class WorksTabView : public QWidget {
  Q_OBJECT
  public:
    WorksTabView(WorkFilterModelCollection * filter_model_collection, QWidget * parent = NULL);
    virtual ~WorksTabView();
    void read_settings();
  public slots:
    void select_work(int work_id);
    void new_tab();
    void write_settings();
  signals:
    void work_selected(int work);
  private:
    QTabWidget * mTabWidget;
    WorkFilterModelCollection * mFilterModelCollection;
    WorkDBView * mAllView;
    //QList<FilteredDBView *> mDBViews;
};

#endif
