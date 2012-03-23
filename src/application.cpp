#include "application.hpp"
#include "mixerpanel.hpp"
#include "player_view.hpp"
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

#include <QSlider>
#include <QFile>
#include <QWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QSqlRecord>

using namespace DataJockey;

namespace {
   const QString cFileQueryString(
         "select audio_files.location audio_file, annotation_files.location beat_file\n"
         "from audio_works\n"
         "\tjoin audio_files on audio_files.id = audio_works.audio_file_id\n"
         "\tjoin annotation_files on annotation_files.audio_work_id = audio_works.id\n"
         "where audio_works.id = ");

   const QString cWorkInfoQueryString(
         "select audio_works.name title,\n"
         "\tartists.name\n"
         "artist from audio_works"
         "\tinner join artist_audio_works on artist_audio_works.audio_work_id = audio_works.id\n"
         "\tinner join artists on artists.id = artist_audio_works.artist_id\n"
         "where audio_works.id = ");
}

Application::Application(int & argc, char ** argv) :
   QApplication(argc, argv),
   mCurrentwork(0)
{
   Model::db::setup("QMYSQL", "datajockey", "developer", "pass");
   mFileQuery = new QSqlQuery("", Model::db::get());
   mWorkInfoQuery = new QSqlQuery("", Model::db::get());

   mAudioModel = Audio::AudioModel::instance();
   mTop = new QWidget(0, Qt::Window);
   mMixerPanel = new View::MixerPanel(mTop);

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
         SIGNAL(player_toggled(int, QString, bool)),
         mAudioModel,
         SLOT(player_set(int, QString, bool)));
   QObject::connect(mMixerPanel,
         SIGNAL(player_value_changed(int, QString, int)),
         mAudioModel,
         SLOT(player_set(int, QString, int)));

   QObject::connect(mAudioModel,
         SIGNAL(player_toggled(int, QString, bool)),
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
         SIGNAL(player_triggered(int, QString)),
         SLOT(relay_player_trigger(int, QString)));

   mAudioModel->master_set("crossfade", true);
   mAudioModel->set_master_cross_fade_players(0, 1);
   mAudioModel->master_set("crossfade_position", (int)one_scale / 2);
   mAudioModel->master_set("bpm", 120.0);


   //mAudioModel->set_player_buffers(0,
         //"/media/x/music/dj_slugo/dance_mania_ghetto_classics_vol_1/11-freaky_ride.flac",
         //"/media/x/datajockey_annotation/recovered/3981-dj_slugo-dance_mania_ghetto_classics_vol_1-freaky_ride.yaml");
   //mAudioModel->set_player_buffers(1,
         //"/media/x/music/low_end_theory/3455/07-suck_it.flac",
         //"/media/x/datajockey_annotation/recovered/3973-low_end_theory-3455-suck_it.yaml");

   TagModel * tag_model = new TagModel(Model::db::get(), mTop);
   WorkTableModel * work_table_model = new WorkTableModel(Model::db::get(), mTop);
	WorkFilterModelProxy * filtered_work_model = new WorkFilterModelProxy(work_table_model);
   WorkDetailView * work_detail = new WorkDetailView(tag_model, Model::db::get(), mTop);
   WorkDBView * work_db_view = new WorkDBView(filtered_work_model, mTop);
   TagEditor * tag_editor = new TagEditor(tag_model, mTop);

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


   QFile file(":/style.qss");
   if(file.open(QFile::ReadOnly)){
      QString styleSheet = QLatin1String(file.readAll());
      this->setStyleSheet(styleSheet);
   }

   QTabWidget * left_tab_view = new QTabWidget(mTop);
   left_tab_view->addTab(mMixerPanel, "mixer");
   left_tab_view->addTab(tag_editor, "tags");
   left_tab_view->addTab(filter_list_view, "filters");

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

   OscThread * osc_thread = new OscThread(12000);

   mAudioModel->start_audio();
   osc_thread->start();

   mTop->setLayout(top_layout);
   mTop->show();
}

void Application::pre_quit_actions() {
   mAudioModel->stop_audio();
   Model::db::close();
}

void Application::select_work(int work_id) {
   mCurrentwork = work_id;
}

#include <iostream>
using namespace std;

void Application::set_player_trigger(int player_index, QString name) {
   if (name != "load")
      return;

   //build up query
   QString fileQueryStr(cFileQueryString);
   QString workQueryStr(cWorkInfoQueryString);
   QString id;
   id.setNum(mCurrentwork);
   fileQueryStr.append(id);
   workQueryStr.append(id);
   //execute
   mFileQuery->exec(fileQueryStr);
   QSqlRecord rec = mFileQuery->record();
   int audioFileCol = rec.indexOf("audio_file");
   int beatFileCol = rec.indexOf("beat_file");

   if(mFileQuery->first()){
      QString audiobufloc = mFileQuery->value(audioFileCol).toString();
      QString beatbufloc = mFileQuery->value(beatFileCol).toString();

      mAudioModel->set_player_buffers(player_index,
            audiobufloc,
            beatbufloc);

      mWorkInfoQuery->exec(workQueryStr);

      if(mWorkInfoQuery->first()){
         rec = mWorkInfoQuery->record();
         int titleCol = rec.indexOf("title");
         int artistCol = rec.indexOf("artist");
         //XXX invoke or is this okay?
         mMixerPanel->player_set(player_index,
               "song_description",
               mWorkInfoQuery->value(artistCol).toString() + "\n" + mWorkInfoQuery->value(titleCol).toString());
      } else {
         mMixerPanel->player_set(player_index, "song_description", "unknown");
      }
   }
}

void Application::relay_player_trigger(int player_index, QString name) {
   if (name == "beat_buffer_changed") {
      //XXX check bounds?
      mMixerPanel->player_set_beat_buffer(player_index, mAudioModel->player_beat_buffer(player_index));
   }
}

