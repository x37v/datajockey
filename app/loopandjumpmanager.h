#ifndef LOOPANDJUMPMANAGER_H
#define LOOPANDJUMPMANAGER_H

#include <QObject>
#include "annotation.hpp"
#include "audio/audiobuffer.hpp"
#include "defines.hpp"
#include "db.h"

struct LoopAndJumpPlayerData;

class LoopAndJumpManager : public QObject {
  Q_OBJECT
  public:
    LoopAndJumpManager(QObject * parent = nullptr);
    void setDB(DB * db);
    void saveData();
  public slots:
    void playerTrigger(int player, QString name);
    void playerSetValueInt(int player, QString name, int v);
    void playerLoad(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer);

  signals:
    //player, type [loop or jump], entry is a slot number, might overwrite an existing one
    void entryUpdated(int player_index, dj::loop_and_jump_type_t type, int entry_index, int frame_start, int frame_end);
    void entriesCleared(int player_index);
    void entryCleared(int player_index, int entry_index);

    void playerValueChangedInt(int player, QString name, int value);
  private:
    void clearEntry(int player, int entry_index);
    QList<LoopAndJumpPlayerData *> mPlayerData;
    DB * mDB = nullptr;
    void saveData(int player);
    void loadData(int player);
    QString yamlData(int player);
};

#endif
