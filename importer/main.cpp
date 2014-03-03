#include <QCoreApplication>
#include <QCommandLineParser>
#include "fileprocessor.h"
#include <QTimer>

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  QCoreApplication::setApplicationName("datajockey_importer");
  QCoreApplication::setApplicationVersion("1.0git"); //XXX deal with it
  QCoreApplication::setOrganizationName("xnor");
  QCoreApplication::setOrganizationDomain("x37v.info");

  QCommandLineParser parser;
  parser.setApplicationDescription("DataJockey audio file importer");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument("files", QCoreApplication::translate("main", "The files to process."));

  parser.process(a);

  QStringList files = parser.positionalArguments();
  FileProcessor * processor = new FileProcessor;
  processor->addFiles(files);

  QObject::connect(processor, &FileProcessor::complete, &a, QCoreApplication::quit);
  QTimer::singleShot(0, processor, SLOT(process()));

  return a.exec();
}
