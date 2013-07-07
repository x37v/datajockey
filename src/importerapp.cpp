#include "importerapp.hpp"
#include "db.hpp"
#include "config.hpp"
#include "beatbuffer.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <QStringList>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMetaObject>
#include <QMetaType>
#include <QTimer>

namespace po = boost::program_options;
using std::cout;
using std::endl;
using dj::audio::BeatBufferPtr;

ImporterApplication::ImporterApplication(int& argc, char ** argv) : QCoreApplication(argc, argv) {
  qRegisterMetaType<BeatBufferPtr>("BeatBufferPtr");

  dj::Configuration * config = dj::Configuration::instance();
  dj::model::DB * db = new dj::model::DB(
      config->db_adapter(),
      config->db_name(),
      config->db_username(),
      config->db_password(),
      config->db_port(),
      config->db_host(),
      this);

  mImporter = new dj::util::Importer(db, this);
  QObject::connect(mImporter, SIGNAL(finished()), SLOT(quit()));
  QObject::connect(this, SIGNAL(aboutToQuit()), SLOT(cleanup()));

  QObject::connect(mImporter, SIGNAL(imported_file(QString, int)), SLOT(report_success(QString, int)));
  QObject::connect(mImporter, SIGNAL(import_failed(QString, QString)), SLOT(report_failure(QString, QString)));

  //create the annotation directory if it doesn't already exist
  QString annotation_dir_name = config->annotation_dir();
  QDir annotation_dir(annotation_dir_name);
  if (!annotation_dir.exists()) {
    if (!annotation_dir.mkpath(annotation_dir.path()))
      throw(std::runtime_error("cannot create path " + annotation_dir.path().toStdString()));
  }
}

void ImporterApplication::import_paths(QStringList paths, QList<QRegExp> ignore_patterns) {
  mImportPaths = paths;
  mIgnorePatterns = ignore_patterns;

  //actually trigger the import through the event mechanism so that the finished signal etc
  //is sent correctly
  QTimer::singleShot(0, this, SLOT(import()));
}

void ImporterApplication::import() {
  mImporter->import(mImportPaths, true, mIgnorePatterns);
}

void ImporterApplication::cleanup() {
  //XXX close db?
}

void ImporterApplication::report_success(QString path, int work_id) {
  cout << "imported: " << qPrintable(path) << endl;
}

void ImporterApplication::report_failure(QString path, QString message) {
  cout << "failed to import: " << qPrintable(path) << endl;
  if (!message.isEmpty())
    cout << "\t" << qPrintable(message) << endl;
}

int main(int argc, char * argv[]) {
  try {
    dj::Configuration * config = dj::Configuration::instance();
    QList<QRegExp> ignore_patterns;

    po::options_description generic_options("Allowed options");
    generic_options.add_options()
      ("help,h", "print this help message")
      ("force,f", "non interactive, just import without any prompting")
      ("config,c", po::value<std::string>(), "specify a config file")
      ("ignore,d", po::value<std::vector<std::string> >(), "specify a pattern to ignore for import")
      ;

    // Hidden options, will be allowed both on command line and
    // in config file, but will not be shown to the user.
    po::options_description hidden_options("Hidden options");
    hidden_options.add_options()
      ("input-file", po::value< std::vector<std::string> >(), "input file")
      ;

    po::positional_options_description p;
    p.add("input-file", -1);

    po::options_description cmdline_options;
    cmdline_options.add(generic_options).add(hidden_options);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
        options(cmdline_options).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
      cout << endl << "usage: " << argv[0] << " [options] file_or_directory_path1 [file_path2 ..]" << endl << endl;
      cout << generic_options << endl;
      return 1;
    }

    if (!vm.count("input-file")) {
      cout << "you must provide some input files" << endl;
      cout << endl << "usage: " << argv[0] << " [options] file_or_directory_path1 [file_path2 ..]" << endl << endl;
      cout << generic_options << endl;
      return 1;
    }

    if (vm.count("config"))
      config->load_file(QString::fromStdString(vm["config"].as<std::string>()));
    else
      config->load_default();

    //grab the ignore patterns from our config file
    QString ignore;
    foreach (ignore, config->import_ignores())
      ignore_patterns << QRegExp(ignore);
    if (vm.count("ignore")) {
      std::vector<std::string> v = vm["ignore"].as<std::vector<std::string> >();
      for (std::vector<std::string>::iterator it = v.begin(); it != v.end(); it++)
        ignore_patterns << QRegExp(QString::fromStdString(*it));
    }

    std::vector<std::string> import_paths = vm["input-file"].as< std::vector<std::string> >();
    if (!vm.count("force")) {
      cout << "Initiating a recursive import of the following paths: " << endl;
      for(std::vector<std::string>::iterator it = import_paths.begin(); it != import_paths.end(); it++)
        cout << "\t" << *it << endl;

      //mangle the db adapter name
      QString db_adapter = config->db_adapter();
      db_adapter = db_adapter.right(db_adapter.size() - 1).toLower();
      cout << endl << "Using " << config->db_name().toStdString() << " " << db_adapter.toStdString() << " database " << endl;

      if (!ignore_patterns.isEmpty()) {
        cout << "Ignoring files that match the following patterns:" << endl;
        QRegExp regex;
        foreach (regex, ignore_patterns) {
          cout << "\t" << regex.pattern().toStdString() << endl;
        }
        cout << endl;
      }

      cout << "Note, files that are already in the database will be ignored." << endl;
      cout << "Type 'yes' to continue or anything else to exit" << endl;

      std::string response;
      std::cin >> response;
      std::transform(response.begin(), response.end(), response.begin(), tolower);

      if (response.find("yes") == std::string::npos) {
        cout << "import aborted" << endl;
        return 1;
      }
    }

    int fake_argc = 1; //only passing the program name because we use the rest with boost
    ImporterApplication app(fake_argc, argv);

    QStringList qpaths;
    for(std::vector<std::string>::iterator it = import_paths.begin(); it != import_paths.end(); it++)
      qpaths << QString::fromStdString(*it);

    app.import_paths(qpaths, ignore_patterns);
    app.exec();

  } catch(std::exception& e) {
    cout << "error ";
    cout << e.what() << endl;
    return 1;
  }
}
