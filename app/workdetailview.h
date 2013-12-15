#ifndef WORKDETAILVIEW_H
#define WORKDETAILVIEW_H

#include <QWidget>

namespace Ui {
class WorkDetailView;
}

class WorkDetailView : public QWidget
{
    Q_OBJECT
    
public:
    explicit WorkDetailView(QWidget *parent = 0);
    ~WorkDetailView();
    
private:
    Ui::WorkDetailView *ui;
};

#endif // WORKDETAILVIEW_H
