#ifndef MIXERPANELVIEW_H
#define MIXERPANELVIEW_H

#include <QWidget>

namespace Ui {
class MixerPanelView;
}

class MixerPanelView : public QWidget
{
    Q_OBJECT
    
public:
    explicit MixerPanelView(QWidget *parent = 0);
    ~MixerPanelView();
    
private:
    Ui::MixerPanelView *ui;
};

#endif // MIXERPANELVIEW_H
