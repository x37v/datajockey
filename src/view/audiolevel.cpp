#include "audiolevel.hpp"
#include <QColor>
#include <QPainter>

using namespace DataJockey::View;

AudioLevel::AudioLevel(QWidget * parent) :
   QWidget(parent),
   mPercent(0),
   mPen(QColor::fromRgb(255,0,0)),
   mBrush(QColor::fromRgb(255,0,0))
{
   setBackgroundRole(QPalette::Base);
   setAutoFillBackground(true);
}

QSize AudioLevel::minimumSizeHint() const { return QSize(4, 20); }
QSize AudioLevel::sizeHint() const { return QSize(4,100); }

#include <iostream>
using namespace std;

void AudioLevel::set_level(int percent) {
   mPercent = percent;
   //cout << percent << endl;
   update();
}

void AudioLevel::paintEvent(QPaintEvent * /* event */) {
   QPainter painter(this);
   painter.setPen(mPen);
   painter.setBrush(mBrush);

   QRect rect = this->rect();
   int new_height = static_cast<int>(mPercent * rect.height() / 100.0);
   rect.translate(0, rect.height() - new_height);
   rect.setHeight(new_height);

   painter.drawRect(rect);
}

