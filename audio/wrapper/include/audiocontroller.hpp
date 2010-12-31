#ifndef DATAJOCKEY_AUDIO_CONTROLLER_HPP
#define DATAJOCKEY_AUDIO_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QMutex>
#include <QMap>
#include <QPair>
#include "master.hpp"
#include "audioio.hpp"
#include "timepoint.hpp"
#include "player.hpp"
#include "audiobuffer.hpp"
#include "beatbuffer.hpp"
#include "scheduler.hpp"
#include <vector>

namespace DataJockey {
   namespace Audio {
      class AudioController : public QObject {
         Q_OBJECT
         private:
            //forward declarations
            class PlayerClearBuffersCommand;
            class PlayerState;
            class ConsumeThread;
            class AudioLoaderThread;

            //singleton
            AudioController();
            AudioController(const AudioController&);
            AudioController& operator=(const AudioController&);
            ~AudioController();
            static AudioController * cInstance;
         public:
            static AudioController * instance();
            //this is the value we use to scale from an int to a double, this
            //represents 'one' as a double in int terms.
            const static unsigned int one_scale;

            //getters
            //player
            bool player_pause(int player_index);
            bool player_cue(int player_index);
            //void player_out_state(int player_index, Audio::Player::out_state_t val);
            //void player_stretch_method(int player_index, Audio::Player::stretch_method_t val);
            bool player_mute(int player_index);
            bool player_sync(int player_index);
            bool player_loop(int player_index);
            int player_volume(int player_index);
            int player_play_speed(int player_index);
            //void player_position(int player_index);
            //void player_start_position(int player_index);
            //void player_end_position(int player_index);
            //void player_loop_start_position(int player_index);
            //void player_loop_end_position(int player_index);
            QString player_audio_file(int player_index);

         public slots:
            void set_player_pause(int player_index, bool pause);
            void set_player_cue(int player_index, bool val);
            //void set_player_out_state(int player_index, Audio::Player::out_state_t val);
            //void set_player_stretch_method(int player_index, Audio::Player::stretch_method_t val);
            void set_player_mute(int player_index, bool val);
            void set_player_sync(int player_index, bool val);
            void set_player_loop(int player_index, bool val);
            void set_player_volume(int player_index, int val);
            void set_player_play_speed(int player_index, int val);
            void set_player_position(int player_index, const DataJockey::Audio::TimePoint &val, bool absolute = true);
            void set_player_position_relative(int player_index, const DataJockey::Audio::TimePoint &val);
            void set_player_position_frame(int player_index, unsigned long frame, bool absolute = true);
            void set_player_start_position(int player_index, const DataJockey::Audio::TimePoint &val);
            void set_player_end_position(int player_index, const DataJockey::Audio::TimePoint &val);
            void set_player_loop_start_position(int player_index, const DataJockey::Audio::TimePoint &val);
            void set_player_loop_end_position(int player_index, const DataJockey::Audio::TimePoint &val);

            void set_player_audio_file(int player_index, QString location);
            void set_player_clear_buffers(int player_index);

            void set_master_volume(int val);
            void set_master_cue_volume(int val);
            void set_master_cross_fade_enable(bool enable);
            void set_master_cross_fade_position(int val);
            void set_master_cross_fade_players(int left, int right);

            //start and stop the audio processing
            void start_audio();
            void stop_audio();

         signals:
            void player_pause_changed(int player_index, bool pause);
            void player_cue_changed(int player_index, bool val);
            void player_mute_changed(int player_index, bool val);
            void player_sync_changed(int player_index, bool val);
            void player_loop_changed(int player_index, bool val);
            void player_volume_changed(int player_index, int val);
            void player_play_speed_changed(int player_index, int val);
            void player_position_changed(int player_index, const DataJockey::Audio::TimePoint &val);
            void player_audio_file_cleared(int player_index);
            void player_audio_file_load_progress(int player_index, int percent);
            void player_audio_file_changed(int player_index, QString location);

            //only for internal use
            void player_audio_file_changed_relay(int, QString);

         protected slots:
            //relay methods are called with queued connections across threads so that
            //they relay signals into the main thread, they simply emit signals
            void relay_audio_file_load_progress(QString fileName, int percent);

         protected:
            friend class PlayerClearBuffersCommand;
            friend class AudioLoaderThread;
            void decrement_audio_file_reference(QString fileName);
         private:
            //pointers to other internal singletons
            DataJockey::Audio::AudioIO * mAudioIO;
            DataJockey::Audio::Master * mMaster;

            unsigned int mNumPlayers;
            std::vector<AudioLoaderThread *> mThreadPool;

            //execute/consume the scheduler's done actions
            ConsumeThread * mConsumeThread;

            //maintain player state information
            std::vector<PlayerState *> mPlayerStates;

            //make sure that player states/audio buffer manager access is thread safe
            QMutex mPlayerStatesMutex;

            //manage audio buffers
            //filename => [refcount, buffer pointer]
            QMap<QString, QPair<int, DataJockey::Audio::AudioBuffer *> > mAudioBufferManager;

            //**** private methods

            //returns true if the buffer is actually loaded anywhere
            //this is called from one of many audio loader threads
            bool audio_file_load_complete(QString fileName, DataJockey::Audio::AudioBuffer * buffer);

            //convenience method for executing commands in the master's scheduler
            void queue_command(DataJockey::Audio::Command * cmd);

            void set_player_audio_buffer(int player_index, DataJockey::Audio::AudioBuffer * buf);
            void set_player_beat_buffer(int player_index, DataJockey::Audio::BeatBuffer * buf);
      };
   }
}

#endif
