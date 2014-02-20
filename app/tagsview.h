#ifndef TAGSVIEW_H
#define TAGSVIEW_H

#include <QWidget>
#include <QAbstractItemModel>
#include <QModelIndex>

namespace Ui {
class TagsView;
}

class DB;
class QKeyEvent;
class TagsView : public QWidget
{
  Q_OBJECT
  public:
    explicit TagsView(QWidget *parent = 0);
    ~TagsView();
    void setModel(QAbstractItemModel * model);
    virtual void keyPressEvent(QKeyEvent * event);
  public slots:
    void createTag();
  signals:
    void newTagRequested(QString name, QModelIndex parent);
    void tagDeleteRequested(QModelIndexList tags);

  private:
    Ui::TagsView *ui;
};

#endif // TAGSVIEW_H
