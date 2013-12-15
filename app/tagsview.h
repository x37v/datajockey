#ifndef TAGSVIEW_H
#define TAGSVIEW_H

#include <QWidget>

namespace Ui {
class TagsView;
}

class TagsView : public QWidget
{
    Q_OBJECT
    
public:
    explicit TagsView(QWidget *parent = 0);
    ~TagsView();
    
private:
    Ui::TagsView *ui;
};

#endif // TAGSVIEW_H
