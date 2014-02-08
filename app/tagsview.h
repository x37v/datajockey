#ifndef TAGSVIEW_H
#define TAGSVIEW_H

#include <QWidget>
#include <QAbstractItemModel>

namespace Ui {
class TagsView;
}

class TagsView : public QWidget
{
  Q_OBJECT
  public:
    explicit TagsView(QWidget *parent = 0);
    ~TagsView();
    void setModel(QAbstractItemModel * model);

  private:
    Ui::TagsView *ui;
};

#endif // TAGSVIEW_H
