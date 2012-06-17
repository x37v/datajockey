#include "application.hpp"
#include "mixerpanel.hpp"
#include "midimapper.hpp"
#include "playerview.hpp"
#include "audiomodel.hpp"
#include "defines.hpp"
#include "beatbuffer.hpp"
#include "db.hpp"
#include "workdetailview.hpp"
#include "workdbview.hpp"
#include "workfiltermodel.hpp"
#include "worktablemodel.hpp"
#include "tagmodel.hpp"
#include "workfilterlist.hpp"
#include "workfilterlistview.hpp"
#include "workfiltermodel.hpp"
#include "defaultworkfilters.hpp"
#include "tageditor.hpp"
#include "oscreceiver.hpp"
#include "config.hpp"
#include "annotation.hpp"

#include <QSlider>
#include <QFile>
#include <QWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QSqlRelationalTableModel>
#include <QSqlRelation>
#include <QDebug>
#include <stdexcept>
#include <boost/program_options.hpp>

using namespace dj;
using std::endl;
using std::cout;

Application::Application(int & argc, char ** argv) :
   QApplication(argc, argv),
   mCurrentwork(0)
{
   Configuration * config = Configuration::instance();

   model::db::setup(
         config->db_adapter(),
         config->db_name(),
         config->db_username(),
         config->db_password());

   mAudioModel = audio::AudioModel::instance();
   mTop = new QWidget(0, Qt::Window);
   mMixerPanel = new view::MixerPanel(mTop);
   mMIDIMapper = new controller::MIDIMapper(mTop);

   setStyle("plastique");

   QObject::connect(this, SIGNAL(aboutToQuit()), SLOT(pre_quit_actions()));

   QObject::connect(mMixerPanel,
         SIGNAL(master_value_changed(QString, int)),
         mAudioModel,
         SLOT(master_set(QString, int)));
   QObject::connect(mAudioModel,
         SIGNAL(master_value_changed(QString, int)),
         mMixerPanel,
         SLOT(master_set(QString, int)));
   QObject::connect(mMixerPanel,
         SIGNAL(master_value_changed(QString, double)),
         mAudioModel,
         SLOT(master_set(QString, double)));
   QObject::connect(mAudioModel,
         SIGNAL(master_value_changed(QString, double)),
         mMixerPanel,
         SLOT(master_set(QString, double)));
   QObject::connect(mAudioModel,
         SIGNAL(master_value_changed(QString, TimePoint)),
         mMixerPanel,
         SLOT(master_set(QString, TimePoint)));

   //hook the trigger to us for loading only
   QObject::connect(mMixerPanel,
         SIGNAL(player_triggered(int, QString)),
         SLOT(set_player_trigger(int, QString)));

   //hook view into model
   QObject::connect(mMixerPanel,
         SIGNAL(player_triggered(int, QString)),
         mAudioModel,
         SLOT(player_trigger(int, QString)));
   QObject::connect(mMixerPanel,
         SIGNAL(player_value_changed(int, QString, bool)),
         mAudioModel,
         SLOT(player_set(int, QString, bool)));
   QObject::connect(mMixerPanel,
         SIGNAL(player_value_changed(int, QString, int)),
         mAudioModel,
         SLOT(player_set(int, QString, int)));

   QObject::connect(mAudioModel,
         SIGNAL(player_value_changed(int, QString, bool)),
         mMixerPanel,
         SLOT(player_set(int, QString, bool)));
   QObject::connect(mAudioModel,
         SIGNAL(player_value_changed(int, QString, int)),
         mMixerPanel,
         SLOT(player_set(int, QString, int)));

   QObject::connect(mAudioModel,
         SIGNAL(player_value_changed(int, QString, QString)),
         mMixerPanel,
         SLOT(player_set(int, QString, QString)));

   QObject::connect(mAudioModel,
         SIGNAL(player_buffers_changed(int, AudioBufferPtr, BeatBufferPtr)),
         mMixerPanel,
         SLOT(player_set_buffers(int, AudioBufferPtr, BeatBufferPtr)));

   mAudioModel->master_set("crossfade", true);
   mAudioModel->set_master_cross_fade_players(0, 1);
   mAudioModel->master_set("crossfade_position", (int)one_scale / 2);
   mAudioModel->master_set("bpm", 120.0);

   //hook up mapper
   //mapper out
   QObject::connect(
         mMIDIMapper, SIGNAL(player_value_changed(int, QString, int)),
         mAudioModel, SLOT(player_set(int, QString, int)),
         Qt::QueuedConnection);
   QObject::connect(
         mMIDIMapper, SIGNAL(player_value_changed(int, QString, bool)),
         mAudioModel, SLOT(player_set(int, QString, bool)),
         Qt::QueuedConnection);
   QObject::connect(
         mMIDIMapper, SIGNAL(player_triggered(int, QString)),
         mAudioModel, SLOT(player_trigger(int, QString)),
         Qt::QueuedConnection);
   QObject::connect(
         mMIDIMapper, SIGNAL(master_value_changed(QString, bool)),
         mAudioModel, SLOT(master_set(QString, bool)),
         Qt::QueuedConnection);
   QObject::connect(
         mMIDIMapper, SIGNAL(master_value_changed(QString, int)),
         mAudioModel, SLOT(master_set(QString, int)),
         Qt::QueuedConnection);
   QObject::connect(
         mMIDIMapper, SIGNAL(master_value_changed(QString, double)),
         mAudioModel, SLOT(master_set(QString, double)),
         Qt::QueuedConnection);

   //mapper in
   QObject::connect(
         mMixerPanel, SIGNAL(player_trigged(int, QString)),
         mMIDIMapper, SLOT(player_trigger(int, QString)),
         Qt::QueuedConnection);
   QObject::connect(
         mAudioModel, SIGNAL(player_value_changed(int, QString, bool)),
         mMIDIMapper, SLOT(player_set(int, QString, bool)),
         Qt::QueuedConnection);
   QObject::connect(
         mAudioModel, SIGNAL(player_value_changed(int, QString, int)),
         mMIDIMapper, SLOT(player_set(int, QString, int)),
         Qt::QueuedConnection);
   QObject::connect(
         mAudioModel, SIGNAL(player_value_changed(int, QString, double)),
         mMIDIMapper, SLOT(player_set(int, QString, double)),
         Qt::QueuedConnection);

   QObject::connect(
         mMixerPanel, SIGNAL(master_trigged(QString)),
         mMIDIMapper, SLOT(player_trigger(QString)),
         Qt::QueuedConnection);
   QObject::connect(
         mAudioModel, SIGNAL(master_value_changed(QString, bool)),
         mMIDIMapper, SLOT(master_set(QString, bool)),
         Qt::QueuedConnection);
   QObject::connect(
         mAudioModel, SIGNAL(master_value_changed(QString, int)),
         mMIDIMapper, SLOT(master_set(QString, int)),
         Qt::QueuedConnection);
   QObject::connect(
         mAudioModel, SIGNAL(master_value_changed(QString, double)),
         mMIDIMapper, SLOT(master_set(QString, double)),
         Qt::QueuedConnection);

   QObject::connect(
         mMixerPanel, SIGNAL(midi_map_triggered()),
         mMIDIMapper, SLOT(map()),
         Qt::QueuedConnection);

   TagModel * tag_model = new TagModel(model::db::get(), mTop);
   //WorkTableModel * work_table_model = new WorkTableModel(model::db::get(), mTop);
	//WorkFilterModelProxy * filtered_work_model = new WorkFilterModelProxy(work_table_model);
   WorkDetailView * work_detail = new WorkDetailView(tag_model, model::db::get(), mTop);

   QSqlRelationalTableModel * rtable_model = new QSqlRelationalTableModel(mTop, model::db::get());
   rtable_model->setTable("works");
   rtable_model->setRelation(model::db::work::temp_table_id_column("audio_file_type"), QSqlRelation("audio_file_types", "id", "name"));
   rtable_model->setRelation(model::db::work::temp_table_id_column("artist"), QSqlRelation("artists", "id", "name"));
   rtable_model->setRelation(model::db::work::temp_table_id_column("album"), QSqlRelation("albums", "id", "name"));
   //XXX quite slow rtable_model->setFilter("works.id IN (select audio_work_id from audio_work_tags where audio_work_tags.tag_id in (2,3,4))");
   rtable_model->select();

   WorkDBView * work_db_view = new WorkDBView(rtable_model, mTop);

   //WorkDBView * work_db_view = new WorkDBView(filtered_work_model, mTop);
   TagEditor * tag_editor = new TagEditor(tag_model, mTop);

   /*
   WorkFilterList * filter_list = new WorkFilterList(work_table_model);
	TagSelectionFilter * tag_filter = new TagSelectionFilter(work_table_model);
	TempoRangeFilter * tempo_filter = new TempoRangeFilter(work_table_model);
   filter_list->addFilter(tag_filter);
   filter_list->addFilter(tempo_filter);
   WorkFilterListView * filter_list_view = new WorkFilterListView(filter_list, mTop);

   //set the filter
   filtered_work_model->setFilter(tag_filter);

   //tag selection
   QObject::connect(
         tag_editor,
         SIGNAL(tagSelectionChanged(QList<int>)),
         tag_filter,
         SLOT(setTags(QList<int>)));
         */

   //work selection
   QObject::connect(
         work_db_view,
         SIGNAL(workSelected(int)),
         work_detail,
         SLOT(setWork(int)));

   QObject::connect(
         work_db_view,
         SIGNAL(workSelected(int)),
         SLOT(select_work(int)));

   /*
   //filter application
   QObject::connect(
         filter_list,
         SIGNAL(selectionChanged(WorkFilterModel *)),
         filtered_work_model,
         SLOT(setFilter(WorkFilterModel *)));
   QObject::connect(
         work_db_view,
         SIGNAL(filter_state_changed(bool)),
         filtered_work_model,
         SLOT(filter(bool)));
   */

   QFile file(":/resources/style.qss");
   if(file.open(QFile::ReadOnly)){
      QString styleSheet = QLatin1String(file.readAll());
      this->setStyleSheet(styleSheet);
   }

   QTabWidget * left_tab_view = new QTabWidget(mTop);
   left_tab_view->addTab(mMixerPanel, "mixer");
   left_tab_view->addTab(tag_editor, "tags");
   //left_tab_view->addTab(filter_list_view, "filters");

   QBoxLayout * left_layout = new QBoxLayout(QBoxLayout::TopToBottom);
   QSplitter * splitter = new QSplitter(Qt::Vertical);
   splitter->addWidget(left_tab_view);
   splitter->addWidget(work_detail);
   left_layout->addWidget(splitter);

   QBoxLayout * top_layout = new QBoxLayout(QBoxLayout::LeftToRight);
   splitter = new QSplitter(Qt::Horizontal);
   QWidget * left = new QWidget;
   left->setLayout(left_layout);
   splitter->addWidget(left);
   splitter->addWidget(work_db_view);
   top_layout->addWidget(splitter);

   OscThread * osc_thread = new OscThread(config->osc_port());

   mAudioModel->start_audio();
   osc_thread->start();

   mMIDIMapper->start();

   mTop->setLayout(top_layout);
   mTop->show();
}

void Application::pre_quit_actions() {
   mAudioModel->stop_audio();
   model::db::close();
}

void Application::select_work(int work_id) {
   mCurrentwork = work_id;
}

void Application::set_player_trigger(int player_index, QString name) {
	if (name != "load")
		return;

	//find the file locations
	QString audio_file;
	QString annotation_file;
	if (!model::db::find_locations_by_id(mCurrentwork, audio_file, annotation_file))
		return;
   mAudioModel->player_load(
         player_index,
         audio_file,
         annotation_file);

	//find the work info
	QString artist_name;
	QString work_title;
	if (model::db::find_artist_and_title_by_id(mCurrentwork, artist_name, work_title)) {
		//XXX invoke or is this okay?
		mMixerPanel->player_set(player_index,
				"song_description",
				artist_name + "\n" + work_title);
	} else
		mMixerPanel->player_set(player_index, "song_description", "unknown");
}

namespace po = boost::program_options;

int main(int argc, char * argv[]){
   try {
      dj::Configuration * config = dj::Configuration::instance();

      po::options_description cmdline_options("Allowed options");
      cmdline_options.add_options()
         ("help,h", "print this help message")
         ("config,c", po::value<std::string>(), "specify a config file")
         ;

      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).
            options(cmdline_options).run(), vm);
      po::notify(vm);

      if (vm.count("help")) {
         cout << endl << "usage: " << argv[0] << " [options] " << endl << endl;
         cout << cmdline_options << endl;
         return 1;
      }

      if (vm.count("config"))
         config->load_file(QString::fromStdString(vm["config"].as<std::string>()));
      else
         config->load_default();

      int fake_argc = 1; //only passing the program name because we use the rest with boost
      dj::Application app(fake_argc, argv);
      app.exec();

   } catch(std::exception& e) {
      cout << "error ";
      cout << e.what() << endl;
      return 1;
   }
}

