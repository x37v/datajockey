#ifndef DATAJOCKEY_AUDIO_MODEL_HPP
#define DATAJOCKEY_AUDIO_MODEL_HPP

#include <QObject>
#include <QString>
#include <QMutex>
#include <QDBusConnection>
#include "master.hpp"
#include "audioio.hpp"
#include "timepoint.hpp"
#include "player.hpp"
#include "audiobuffer.hpp"
#include "beatbuffer.hpp"
#include "loaderthread.hpp"
#include "scheduler.hpp"
#include <vector>

class QTimer;

namespace dj {
   namespace audio {
      class AudioModel : public QObject {
         Q_OBJECT
         Q_CLASSINFO("D-Bus Interface", "org.x37v.datajockey.audio")

         private:
            //forward declarations
            class PlayerSetBuffersCommand;
            class PlayerState;
            class ConsumeThread;
            class QueryPlayState;

            //singleton
            AudioModel();
            AudioModel(const AudioModel&);
            AudioModel& operator=(const AudioModel&);
            ~AudioModel();
            static AudioModel * cInstance;
         public:
            static AudioModel * instance();

         public slots:
            unsigned int sample_rate() const;

            double master_bpm() const;

            unsigned int player_count() const;

            bool player_state_bool(int player_index, QString name);
            int player_state_int(int player_index, QString name);

            //trigger also takes any player_set(..bool) params and toggles them
            void player_trigger(int player_index, QString name);
            void player_set(int player_index, QString name, bool value);
            void player_set(int player_index, QString name, int value);
            void player_set(int player_index, QString name, double value);
            void player_set(int player_index, QString name, dj::audio::TimePoint value);
            void player_load(int player_index, QString audio_file_path, QString annotation_file_path);

            //***** MASTER COMMANDS
            void master_set(QString name, bool value);
            void master_set(QString name, int value);
            void master_set(QString name, double value);

            //start and stop the audio processing
            void start_audio();
            void stop_audio();

         signals:
            void player_value_changed(int player_index, QString name, int value);
            void player_value_changed(int player_index, QString name, QString value);
            void player_value_changed(int player_index, QString name, bool value);
            void player_triggered(int player_index, QString name);
            void player_buffers_changed(int player_index, AudioBufferPtr audio_buffer, BeatBufferPtr beatbuffer);

            void master_value_changed(QString name, int value);
            void master_value_changed(QString name, double value);
            void master_value_changed(QString name, TimePoint timepoint);

         protected slots:
            //only for internal use
            void relay_player_buffers_loaded(int player_index,
                  AudioBufferPtr audio_buffer,
                  BeatBufferPtr beat_buffer);
            void relay_player_value(int player_index, QString name, int value);
            void relay_master_audio_level(int percent);
            void relay_master_position(TimePoint position);

            //relay methods are called with queued connections across threads so that
            //they relay signals into the main thread, they simply emit signals
            void relay_audio_file_load_progress(int player_index, int percent);
            void players_eval_audible();

         private:
            friend class PlayerSetBuffersCommand;
            //pointers to other internal singletons
            dj::audio::AudioIO * mAudioIO;
            dj::audio::Master * mMaster;

            unsigned int mNumPlayers;
            std::vector<LoaderThread *> mThreadPool;
            //we keep a copy of the pointers for each time we push them into
            //the audio thread we remove a copy each time they come out of the
            //audio thread this is how we manage the reference counting and
            //avoid having deallocation happen in the audio thread
            QList<AudioBufferPtr> mPlayingAudioFiles;
            QList<BeatBufferPtr> mPlayingAnnotationFiles;

            //execute/consume the scheduler's done actions
            ConsumeThread * mConsumeThread;

            //maintain player state information
            std::vector<PlayerState *> mPlayerStates;
            typedef QPair<PlayerStateCommand::action_t, PlayerStateCommand::action_t> player_onoff_action_pair_t;
            QHash<QString, player_onoff_action_pair_t> mPlayerStateActionMapping;
            QHash<QString, PlayerDoubleCommand::action_t> mPlayerDoubleActionMapping;
            QHash<QString, PlayerPositionCommand::position_t> mPlayerPositionActionMapping;


            //make sure that player states/audio buffer manager access is thread safe
            QMutex mPlayerStatesMutex;

            //maintain master state info
            double mMasterBPM;
            bool mCrossFadeEnabled;
            int mCrossFadePosition;
            int mCrossFadePlayers[2];

            int mPlayerAudibleThresholdVolume;
            int mCrossfadeAudibleThresholdPosition;

            QTimer * mAudibleTimer;

            //**** private methods

            //returns true if the buffer is actually loaded anywhere
            //this is called from one of many audio loader threads
            bool audio_file_load_complete(QString fileName, dj::audio::AudioBuffer * buffer);

            //convenience method for executing commands in the master's scheduler
            void queue_command(dj::audio::Command * cmd);

            void set_player_audio_file(int player_index, QString location);
            void set_player_audio_buffer(int player_index, dj::audio::AudioBuffer * buf);

            void update_player_state(int player_index, PlayerState * state);

            //not threadsafe, expects to have mutex already locked
            //eq the eq 0 is the lowest, 2 is high, one_scale is the top
            void set_player_eq(int player_index, int band, int value);
            void set_player_position(int player_index, const dj::audio::TimePoint &val, bool absolute = true);
            void set_player_position_frame(int player_index, int frame, bool absolute = true);
            void set_player_position_beat_relative(int player_index, int beats);

            void player_eval_audible(int player_index);
      };
   }
}

#endif
