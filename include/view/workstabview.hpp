#ifndef WORKS_TAB_VIEW_HPP
#define WORKS_TAB_VIEW_HPP

#include <QTabWidget>
#include <QList>

#include "renameabletabbar.hpp"
#include "workdbview.hpp"
#include "filtereddbview.hpp"
#include "workfiltermodelcollection.hpp"

class WorksTabView : public QWidget {
  Q_OBJECT
  public:
    WorksTabView(WorkFilterModelCollection * filter_model_collection, QWidget * parent = NULL);
    virtual ~WorksTabView();
  public slots:
    void select_work(int work_id);
    void add_filter(QString expression = QString(), QString label = QString());
    void write_settings();
  protected slots:
    void read_settings();
  signals:
    void work_selected(int work);
  private:
    void create_filter_tab(QString expression = QString(), QString label = QString());
    RenameableTabWidget * mTabWidget;
    WorkFilterModelCollection * mFilterModelCollection;
    WorkDBView * mAllView;
    QList<FilteredDBView *> mFilterViews;
};

#endif
