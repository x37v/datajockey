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
#include "audioloaderthread.hpp"
#include <vector>

namespace DataJockey {
   class AudioModel : public QObject {
      Q_OBJECT
      private:
         //forward declarations
         class PlayerClearBuffersCommand;
         class PlayerState;
         class ConsumeThread;

         //singleton
         AudioModel();
         AudioModel(const AudioModel&);
         AudioModel& operator=(const AudioModel&);
         ~AudioModel();
         static AudioModel * cInstance;
      public:
         static AudioModel * instance();
         //this is the value we use to scale from an int to a double, this
         //represents 'one' as a double in int terms.
         const static unsigned int one_scale;

         friend class Internal::AudioLoaderThread;
      public slots:
         void set_player_pause(int player_index, bool pause);
         void set_player_cue(int player_index, bool val);
         //void set_player_out_state(int player_index, Internal::Player::out_state_t val);
         //void set_player_stretch_method(int player_index, Internal::Player::stretch_method_t val);
         void set_player_mute(int player_index, bool val);
         void set_player_sync(int player_index, bool val);
         void set_player_loop(int player_index, bool val);
         void set_player_volume(int player_index, int val);
         void set_player_play_speed(int player_index, int val);
         void set_player_position(int player_index, const DataJockey::TimePoint &val);
         void set_player_start_position(int player_index, const DataJockey::TimePoint &val);
         void set_player_end_position(int player_index, const DataJockey::TimePoint &val);
         void set_player_loop_start_position(int player_index, const DataJockey::TimePoint &val);
         void set_player_loop_end_position(int player_index, const DataJockey::TimePoint &val);

         void set_player_audio_file(int player_index, QString location);
         void set_player_clear_buffers(int player_index);

         void set_master_volume(int val);
         void set_master_cue_volume(int val);
         void set_master_cross_fade_enable(bool enable);
         void set_master_cross_fade_position(int val);
         void set_master_cross_fade_players(int left, int right);

      signals:
         void player_pause_changed(int player_index, bool pause);
         void player_cue_changed(int player_index, bool val);
         void player_mute_changed(int player_index, bool val);
         void player_sync_changed(int player_index, bool val);
         void player_loop_changed(int player_index, bool val);
         void player_volume_changed(int player_index, int val);
         void player_play_speed_changed(int player_index, int val);
         void player_position_changed(int player_index, const DataJockey::TimePoint &val);
         void player_audio_file_cleared(int player_index);
         void player_audio_file_load_progress(int player_index, int percent);
         void player_audio_file_changed(int player_index, QString location);

      protected:
         friend class PlayerClearBuffersCommand;
         void decrement_audio_file_reference(QString fileName);
      private:
         //pointers to other internal singletons
         DataJockey::Internal::AudioIO * mAudioIO;
         DataJockey::Internal::Master * mMaster;

         unsigned int mNumPlayers;
         std::vector<Internal::AudioLoaderThread *> mThreadPool;

         //execute/consume the scheduler's done actions
         ConsumeThread * mConsumeThread;

         //maintain player state information
         std::vector<PlayerState *> mPlayerStates;

         //make sure that player states/audio buffer manager access is thread safe
         QMutex mPlayerStatesMutex;

         //manage audio buffers
         //filename => [refcount, buffer pointer]
         QMap<QString, QPair<int, DataJockey::AudioBuffer *> > mAudioBufferManager;

         //**** private methods
         
         //relay methods are called with queued connections across threads so that
         //they relay signals into the main thread, they simply emit signals
         void relay_player_audio_file_load_progress(QString fileName, int percent);
         void relay_player_audio_file_changed(int player_index, QString fileName);

         //returns true if the buffer is actually loaded anywhere
         //this is called from one of many audio loader threads
         bool audio_file_load_complete(QString fileName, DataJockey::AudioBuffer * buffer);

         //convenience method for executing commands in the master's scheduler
         void queue_command(DataJockey::Internal::Command * cmd);

         void set_player_audio_buffer(int player_index, AudioBuffer * buf);
         void set_player_beat_buffer(int player_index, BeatBuffer * buf);
   };
}
