#include "mixerpanelview.h"
#include "ui_mixerpanelview.h"

MixerPanelView::MixerPanelView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MixerPanelView)
{
    ui->setupUi(this);
}

MixerPanelView::~MixerPanelView()
{
    delete ui;
}
