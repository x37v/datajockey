#include <QObject>
#include <QString>
#include <QThread>
#include "master.hpp"
#include "audioio.hpp"
#include "timepoint.hpp"
#include "player.hpp"
#include "audiobuffer.hpp"
#include "beatbuffer.hpp"
#include "scheduler.hpp"
#include <vector>

namespace DataJockey {
   class AudioModel;

   namespace Internal {
      class AudioLoaderThread : public QThread {
         public:
            AudioLoaderThread(AudioModel * model, unsigned int player_index);
            void run();
         private:
            AudioModel * mAudioModel;
            unsigned int mPlayerIndex;
      };

      //run thread to execute done actions
      class ConsumeThread : public QThread {
         public:
            ConsumeThread(Scheduler * scheduler);
            void run();
         private:
            Scheduler * mScheduler;
      };
   }

   class AudioModel : public QObject {
      Q_OBJECT
      public:
         AudioModel(unsigned int num_players = 2);
         virtual ~AudioModel();
         //this is the value we use to scale from an int to a double, this
         //represents 'one' as a double in int terms.
         const static unsigned int one_scale;

         friend class Internal::AudioLoaderThread;
      public slots:
         void set_player_pause(int player_index, bool pause);
         //void set_player_out_state(int player_index, Internal::Player::out_state_t val);
         //void set_player_stretch_method(int player_index, Internal::Player::stretch_method_t val);
         void set_player_mute(int player_index, bool val);
         void set_player_sync(int player_index, bool val);
         void set_player_loop(int player_index, bool val);
         void set_player_volume(int player_index, int val);
         void set_player_play_speed(int player_index, int val);
         void set_player_position(int player_index, const TimePoint &val);
         void set_player_start_position(int player_index, const TimePoint &val);
         void set_player_end_position(int player_index, const TimePoint &val);
         void set_player_loop_start_position(int player_index, const TimePoint &val);
         void set_player_loop_end_position(int player_index, const TimePoint &val);
         void set_player_audio_buffer(int player_index, AudioBuffer * buf);
         void set_player_beat_buffer(int player_index, BeatBuffer * buf);

         void set_player_audio_file(int player_index, QString location);

      signals:
         void player_audio_file_load_progress(int player_index, int percent);
      private:
         DataJockey::Internal::AudioIO * mAudioIO;
         DataJockey::Internal::Master * mMaster;
         unsigned int mNumPlayers;
         void queue_command(DataJockey::Internal::Command * cmd);
         std::vector<Internal::AudioLoaderThread *> mThreadPool;
         Internal::ConsumeThread * mConsumeThread;

         void relay_player_audio_file_load_progress(int player_index, int percent);
   };
}
