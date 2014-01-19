#ifndef DATAJOCKEY_AUDIOLEVEL_VIEW_HPP
#define DATAJOCKEY_AUDIOLEVEL_VIEW_HPP

#include <QWidget>
#include <QPen>
#include <QBrush>
#include <QTimer>

class AudioLevelView : public QWidget {
  Q_OBJECT
  public:
    AudioLevelView(QWidget * parent = nullptr);
    QSize minimumSizeHint() const;
    QSize sizeHint() const;

  public slots:
    void setLevel(int percent);

  protected slots:
    void fadeOut();

  protected:
    void paintEvent(QPaintEvent *event);

  private:
    int mPercent;
    int mPercentLast;
    QPen mPen;
    QBrush mBrush;
    QTimer mDrawTimeout;
    QTimer mColorTimeout;
};

#endif
