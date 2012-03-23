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
#include "audiobufferreference.hpp"
#include "beatbuffer.hpp"
#include "scheduler.hpp"
#include <vector>

namespace DataJockey {
   namespace Audio {
      class AudioModel : public QObject {
         Q_OBJECT
         Q_CLASSINFO("D-Bus Interface", "org.x37v.datajockey.audio")

         private:
            //forward declarations
            class PlayerClearBuffersCommand;
            class PlayerState;
            class ConsumeThread;
            class AudioLoaderThread;
            class QueryPlayerStates;

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

            QString player_audio_file(int player_index);
            BeatBuffer player_beat_buffer(int player_index);
            bool player_state_bool(int player_index, QString name);
            int player_state_int(int player_index, QString name);

            void player_trigger(int player_index, QString name);
            void player_set(int player_index, QString name, bool value);
            void player_set(int player_index, QString name, int value);
            void player_set(int player_index, QString name, double value);
            void player_set(int player_index, QString name, DataJockey::Audio::TimePoint value);

            //****** BEAT BUFFER COMMANDS
            //clear a players beat buffer
            void set_player_beat_buffer_clear(int player_index);
            //starts a beat buffer setting transaction
            //the beat buffer isn't actually sent to the player until
            //after the transaction is ended
            void set_player_beat_buffer_begin(int player_index);
            //ends the beat buffer transaction, the transaction is canceled if commit = false
            void set_player_beat_buffer_end(int player_index, bool commit = true);
            //add a beat to the beat buffer
            void set_player_beat_buffer_add_beat(int player_index, double value);
            void set_player_beat_buffer_remove_beat(int player_index, double value);
            void set_player_beat_buffer_update_beat(int player_index, int beat_index, double new_value);

            void set_player_beat_buffer(int player_index, QString buffer_file);
            void set_player_buffers(int player_index, QString audio_file, QString beat_file);

            //***** MASTER COMMANDS
            void master_set(QString name, bool value);
            void master_set(QString name, int value);
            void master_set(QString name, double value);

            void set_master_cross_fade_players(int left, int right);

            //start and stop the audio processing
            void start_audio();
            void stop_audio();

         signals:
            void player_value_changed(int player_index, QString name, int value);
            void player_value_changed(int player_index, QString name, QString value);
            void player_toggled(int player_index, QString name, bool value);
            void player_triggered(int player_index, QString name);

            void master_value_changed(QString name, int value);
            void master_value_changed(QString name, double value);

         protected slots:
            //only for internal use
            void relay_player_audio_file_changed(int player_index, QString fileName);
            void relay_player_position_changed(int player_index, int frame_index);

            //relay methods are called with queued connections across threads so that
            //they relay signals into the main thread, they simply emit signals
            void relay_audio_file_load_progress(QString fileName, int percent);

         private:
            friend class PlayerClearBuffersCommand;
            friend class AudioLoaderThread;
            //pointers to other internal singletons
            DataJockey::Audio::AudioIO * mAudioIO;
            DataJockey::Audio::Master * mMaster;

            unsigned int mNumPlayers;
            std::vector<AudioLoaderThread *> mThreadPool;

            //execute/consume the scheduler's done actions
            ConsumeThread * mConsumeThread;

            //maintain player state information
            std::vector<PlayerState *> mPlayerStates;
            typedef QPair<PlayerStateCommand::action_t, PlayerStateCommand::action_t> player_onoff_action_pair_t;
            QMap<QString, player_onoff_action_pair_t> mPlayerStateActionMapping;
            QMap<QString, PlayerDoubleCommand::action_t> mPlayerDoubleActionMapping;
            QMap<QString, PlayerPositionCommand::position_t> mPlayerPositionActionMapping;

            //make sure that player states/audio buffer manager access is thread safe
            QMutex mPlayerStatesMutex;

            double mMasterBPM;

            //**** private methods

            //returns true if the buffer is actually loaded anywhere
            //this is called from one of many audio loader threads
            bool audio_file_load_complete(QString fileName, DataJockey::Audio::AudioBuffer * buffer);

            //convenience method for executing commands in the master's scheduler
            void queue_command(DataJockey::Audio::Command * cmd);

            void set_player_audio_file(int player_index, QString location);
            void set_player_audio_buffer(int player_index, DataJockey::Audio::AudioBuffer * buf);

            void update_player_state(int player_index, PlayerState * state);
            void player_clear_buffers(int player_index);

            //not threadsafe, expects to have mutex already locked
            //eq the eq 0 is the lowest, 2 is high, one_scale is the top
            void set_player_eq(int player_index, int band, int value);
            void set_player_position(int player_index, const DataJockey::Audio::TimePoint &val, bool absolute = true);
            void set_player_position_frame(int player_index, int frame, bool absolute = true);
            void set_player_position_beat_relative(int player_index, int beats);
      };
   }
}

#endif
