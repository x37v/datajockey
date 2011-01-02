#include "audiocontroller.hpp"
#include <QObject>
#include <QApplication>

int main(int argc, char* argv[]) {
   QApplication app(argc, argv);

   DataJockey::Audio::AudioController * audio_controller = DataJockey::Audio::AudioController::instance();
   //audio_controller->setParent(&app);

   QDBusConnection dbus_connection = QDBusConnection::sessionBus();
   dbus_connection.registerObject("/audio", audio_controller, QDBusConnection::ExportAllContents);
   dbus_connection.registerService("org.x37v.datajockey");

   audio_controller->start_audio();

   QObject::connect(&app, SIGNAL(aboutToQuit()),
         audio_controller, SLOT(stop_audio()));

   app.exec();
   exit(0);
}
