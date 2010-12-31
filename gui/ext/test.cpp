#include "waveformview.hpp"
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>

int main(int argc, char * argv[]) {
	QApplication app(argc, argv);

   WaveFormView * w = new WaveFormView;
   QGraphicsScene * s = new QGraphicsScene;
   QGraphicsView * v = new QGraphicsView(s);

   s->addItem(w);

   v->show();
   v->scale(1,0.5);
   v->centerOn(2 * 44100, 0);
   //v->rotate(90);

   app.exec();

   return 0;
}
