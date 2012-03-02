#include "player_view.hpp"
#include <QPushButton>
#include <QGridLayout>

using namespace DataJockey::View;

struct button_info {
   int row;
   int col;
   bool checkable;
   QString label;
   QString name;
};

Player::Player(QWidget * parent) : QWidget(parent) {
   button_info items[] = {
      {0, 0, false, "ld", "load"},
      {0, 2, false, "rs", "reset"},
      {1, 0, true,  "cu", "cue"},
      {1, 1, true,  "sy", "sync"},
      {1, 2, true,  "pl", "play"},
      {2, 0, false, "<<", "seek_back"},
      {2, 2, false, ">>", "seek_forward"},
   };

   mTopLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
   QGridLayout * mButtonLayout = new QGridLayout();

   for (unsigned int i = 0; i < sizeof(items) / sizeof(button_info); i++) {
      QPushButton * btn = new QPushButton(items[i].label, this);
      btn->setCheckable(items[i].checkable);
      mButtons.insert(items[i].name, btn);
      mButtonLayout->addWidget(btn, items[i].row, items[i].col);
   }
   mTopLayout->addLayout(mButtonLayout);

   setLayout(mTopLayout);
}

