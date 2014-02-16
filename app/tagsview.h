#ifndef TAGSVIEW_H
#define TAGSVIEW_H

#include <QWidget>
#include <QAbstractItemModel>
#include <QModelIndex>

namespace Ui {
class TagsView;
}

class DB;
class TagsView : public QWidget
{
  Q_OBJECT
  public:
    explicit TagsView(QWidget *parent = 0);
    ~TagsView();
    void setModel(QAbstractItemModel * model);
  public slots:
    void createTag();
  signals:
    void newTagRequested(QString name, QModelIndex parent);

  private:
    Ui::TagsView *ui;
};

#endif // TAGSVIEW_H
