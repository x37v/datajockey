#include "playerview.h"
#include "ui_playerview.h"

PlayerView::PlayerView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayerView)
{
    ui->setupUi(this);
}

PlayerView::~PlayerView()
{
    delete ui;
}
