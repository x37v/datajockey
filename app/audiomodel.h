#ifndef AUDIOMODEL_H
#define AUDIOMODEL_H
#include <QObject>
#include "audioio.hpp"

class AudioModel : public QObject
{
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

    bool inRange(int player);
    void queue(djaudio::Command * cmd);
};

#endif // AUDIOMODEL_H
