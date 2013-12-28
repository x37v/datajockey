#ifndef AUDIOMODEL_H
#define AUDIOMODEL_H

#include <QObject>
#include <QList>
#include "audioio.hpp"

class ConsumeThread;
class AudioModel : public QObject {
  Q_OBJECT
  public:
    explicit AudioModel(QObject *parent = 0);
    ~AudioModel();
  signals:
    void playerValueChangedDouble(int player, QString name, double v);
    void playerValueChangedInt(int player, QString name, int v);
    void playerValueChangedBool(int player, QString name, bool v);

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
  private:
    djaudio::AudioIO * mAudioIO = nullptr;
    djaudio::Master * mMaster = nullptr;
    int mNumPlayers = 2;
    ConsumeThread * mConsumeThread;

    //holding on to a reference so that we only dealloc in the GUI thread
    QList<djaudio::AudioBufferPtr> mAudioBuffers;
    QList<djaudio::BeatBufferPtr> mBeatBuffers;

    bool inRange(int player);
    void queue(djaudio::Command * cmd);
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

#endif // AUDIOMODEL_H
