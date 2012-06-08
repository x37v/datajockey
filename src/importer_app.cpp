#include "importer_app.hpp"
#include "db.hpp"
#include "config.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <QStringList>

namespace po = boost::program_options;
using std::cout;
using std::endl;

ImporterApplication::ImporterApplication(int& argc, char ** argv) : QCoreApplication(argc, argv) {
   mImporter = new dj::util::Importer(this);
   QObject::connect(mImporter, SIGNAL(finished()), SLOT(quit()));
   QObject::connect(this, SIGNAL(aboutToQuit()), SLOT(cleanup()));

   dj::Configuration * config = dj::Configuration::instance();
   dj::model::db::setup(
         config->db_adapter(),
         config->db_name(),
         config->db_username(),
         config->db_password());
}

void ImporterApplication::import_paths(std::vector<std::string>& paths) {
   QStringList qpaths;
   for(std::vector<std::string>::iterator it = paths.begin(); it != paths.end(); it++)
      qpaths << QString::fromStdString(*it);
   mImporter->import(qpaths);
}

void ImporterApplication::cleanup() {
   dj::model::db::close();
}

int main(int argc, char * argv[]) {
   try {
      dj::Configuration * config = dj::Configuration::instance();

      po::options_description generic_options("Allowed options");
      generic_options.add_options()
         ("help,h", "print this help message")
         ("force,f", "non interactive, just import without any prompting")
         ("config,c", po::value<std::string>(), "specify a config file")
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

      std::vector<std::string> import_paths = vm["input-file"].as< std::vector<std::string> >();
      if (!vm.count("force")) {
         cout << "Initiating a recursive import of the following paths: " << endl;
         for(std::vector<std::string>::iterator it = import_paths.begin(); it != import_paths.end(); it++)
            cout << "\t" << *it << endl;

         //mangle the db adapter name
         QString db_adapter = config->db_adapter();
         db_adapter = db_adapter.right(db_adapter.size() - 1).toLower();
         cout << endl << "Using " << config->db_name().toStdString() << " " << db_adapter.toStdString() << " database " << endl;

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
      app.import_paths(import_paths);
      app.exec();

   } catch(std::exception& e) {
      cout << "error ";
      cout << e.what() << endl;
      return 1;
   }
}