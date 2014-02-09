#ifndef AUDIOMODEL_H
#define AUDIOMODEL_H

#include <QObject>
#include <QList>
#include <functional>
#include "audioio.hpp"

class Consumer;
class EngineQueryCommand;
struct PlayerState;
struct EnginePlayerState;

class AudioModel : public QObject {
  Q_OBJECT
  public:
    explicit AudioModel(QObject *parent = 0);
    ~AudioModel();
    int playerCount() const { return mNumPlayers; }
  signals:
    void playerValueChangedDouble(int player, QString name, double v);
    void playerValueChangedInt(int player, QString name, int v);
    void playerValueChangedBool(int player, QString name, bool v);

    void masterValueChangedDouble(QString name, double v);
    void masterValueChangedInt(QString name, int v);
    void masterValueChangedBool(QString name, bool v);

  public slots:
    void playerSetValueDouble(int player, QString name, double v);
    void playerSetValueInt(int player, QString name, int v);
    void playerSetValueBool(int player, QString name, bool v);
    void playerTrigger(int player, QString name);
    void playerLoad(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer);
    void playerClear(int player);

    void masterSetValueDouble(QString name, double v);
    void masterSetValueInt(QString name, int v);
    void masterSetValueBool(QString name, bool v);
    void masterTrigger(QString name);

    //start/stop the audio processing
    void run(bool doit);

    djaudio::AudioIO * audioio() const { return mAudioIO; }
  private:
    djaudio::AudioIO * mAudioIO = nullptr;
    djaudio::Master * mMaster = nullptr;
    int mNumPlayers = 2;
    QThread * mConsumeThread;
    Consumer * mConsumer;
    bool mCueOnLoad = true;

    //holding on to a reference so that we only dealloc in the GUI thread
    QList<djaudio::AudioBufferPtr> mAudioBuffers;
    QList<djaudio::BeatBufferPtr> mBeatBuffers;
    QList<PlayerState *> mPlayerStates;

    QHash<QString, double> mMasterDoubleValue;
    QHash<QString, int> mMasterIntValue;

    bool inRange(int player);
    void queue(djaudio::Command * cmd);
    void playerSet(int player, std::function<djaudio::Command *(PlayerState * state)> func);
    void masterSet(std::function<djaudio::Command *(void)> func);
};


class PlayerSetBuffersCommand : public QObject, public djaudio::PlayerCommand {
  Q_OBJECT
  signals:
    void done(djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer);
  public:
    PlayerSetBuffersCommand(unsigned int idx, djaudio::AudioBuffer * audio_buffer, djaudio::BeatBuffer * beat_buffer);
    virtual ~PlayerSetBuffersCommand();
    virtual void execute();
    virtual void execute_done();
    virtual bool store(djaudio::CommandIOData& data) const;
  private:
    djaudio::AudioBuffer * mAudioBuffer;
    djaudio::BeatBuffer * mBeatBuffer;
    djaudio::AudioBuffer * mOldAudioBuffer;
    djaudio::BeatBuffer * mOldBeatBuffer;
};

class EngineQueryCommand : public QObject, public djaudio::MasterCommand {
  Q_OBJECT
  public:
    EngineQueryCommand(int num_players, QObject * parent = NULL);
    virtual bool delete_after_done();
    virtual void execute();
    virtual void execute_done();
    virtual bool store(djaudio::CommandIOData& /* data */) const;
  signals:
    void playerValueUpdateBool(int player, QString name, bool val);
    void playerValueUpdateInt(int player, QString name, int val);
    void playerValueUpdateDouble(int player, QString name, double val);
    void masterValueUpdateDouble(QString name, double val);
  private:
    QList<EnginePlayerState *> mPlayerStates;
    double mMasterBPM;
    double mMasterVolume;
};

class MasterSyncToPlayerCommand : public QObject, public djaudio::MasterIntCommand {
  Q_OBJECT
  public:
    MasterSyncToPlayerCommand(int value);
    virtual void execute();
    virtual void execute_done();
  signals:
    void masterValueUpdateDouble(QString name, double value);
  private:
    double mBPM;
};

#endif // AUDIOMODEL_H
