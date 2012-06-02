#include "audiolevel.hpp"
#include <QColor>
#include <QPainter>
#include <cmath>

using namespace dj::view;

namespace {
   const int draw_timeout_ms = 100;
   const int color_timeout_ms = 600;
}

AudioLevel::AudioLevel(QWidget * parent) :
   QWidget(parent),
   mPercent(0),
   mPercentLast(0),
   mPen(QColor::fromRgb(0, 255,0)),
   mBrush(QColor::fromRgb(0, 255,0))
{
   setBackgroundRole(QPalette::Base);
   setAutoFillBackground(true);
   mDrawTimeout.setSingleShot(true);
   mColorTimeout.setSingleShot(true);
   QObject::connect(&mDrawTimeout, SIGNAL(timeout()), SLOT(fade_out()));
}

QSize AudioLevel::minimumSizeHint() const { return QSize(4, 20); }
QSize AudioLevel::sizeHint() const { return QSize(4,100); }

void AudioLevel::set_level(int percent) {
   if (percent > 100) {
      mBrush.setColor(QColor::fromRgb(255, 0, 0));
      mPen.setColor(QColor::fromRgb(255, 0, 0));
      mColorTimeout.start(color_timeout_ms);
   }

   if (percent > mPercent) {
      mPercent = percent;
      update();
   }
   mDrawTimeout.start(draw_timeout_ms);
}

void AudioLevel::fade_out() {
   if (mPercentLast > 0) {
      mPercentLast -= 10;
      if (mPercentLast < 0)
         mPercentLast = 0;
      mPercent = mPercentLast;
      update();
      mDrawTimeout.start(draw_timeout_ms);
   }
}

void AudioLevel::paintEvent(QPaintEvent * /* event */) {
   if (!mColorTimeout.isActive()) {
      mBrush.setColor(QColor::fromRgb(0, 255, 0));
      mPen.setColor(QColor::fromRgb(0, 255, 0));
   }

   QPainter painter(this);
   painter.setRenderHint(QPainter::Antialiasing, true);
   painter.setPen(mPen);
   painter.setBrush(mBrush);

   int percent = mPercent;
   if (percent < 1)
      percent = 1;
   else if (percent > 100)
      percent = 100;

   percent = 100 * (log10f(static_cast<float>(percent)) / log10f(100.0));

   QRect rect = this->rect();
   int new_height = static_cast<int>(percent * rect.height() / 100.0);
   rect.translate(0, rect.height() - new_height);
   rect.setHeight(new_height);

   painter.drawRect(rect);

   mPercentLast = mPercent;
   mPercent = 0;
}

