/****************************************************************************
** Meta object code from reading C++ file 'audiocontroller.hpp'
**
** Created: Thu Dec 30 15:33:44 2010
**      by: The Qt Meta Object Compiler version 62 (Qt 4.6.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "audiocontroller.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'audiocontroller.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.6.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DataJockey__Audio__AudioController[] = {

 // content:
       4,       // revision
       0,       // classname
       0,    0, // classinfo
      30,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: signature, parameters, type, tag, flags
      55,   36,   35,   35, 0x05,
     103,   86,   35,   35, 0x05,
     132,   86,   35,   35, 0x05,
     162,   86,   35,   35, 0x05,
     192,   86,   35,   35, 0x05,
     222,   86,   35,   35, 0x05,
     253,   86,   35,   35, 0x05,
     288,   86,   35,   35, 0x05,
     359,  346,   35,   35, 0x05,
     411,  390,   35,   35, 0x05,
     474,  452,   35,   35, 0x05,

 // slots: signature, parameters, type, tag, flags
     513,   36,   35,   35, 0x0a,
     540,   86,   35,   35, 0x0a,
     565,   86,   35,   35, 0x0a,
     591,   86,   35,   35, 0x0a,
     617,   86,   35,   35, 0x0a,
     643,   86,   35,   35, 0x0a,
     670,   86,   35,   35, 0x0a,
     701,   86,   35,   35, 0x0a,
     755,   86,   35,   35, 0x0a,
     815,   86,   35,   35, 0x0a,
     873,   86,   35,   35, 0x0a,
     938,   86,   35,   35, 0x0a,
    1001,  452,   35,   35, 0x0a,
    1036,  346,   35,   35, 0x0a,
    1070, 1066,   35,   35, 0x0a,
    1093, 1066,   35,   35, 0x0a,
    1127, 1120,   35,   35, 0x0a,
    1162, 1066,   35,   35, 0x0a,
    1209, 1198,   35,   35, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_DataJockey__Audio__AudioController[] = {
    "DataJockey::Audio::AudioController\0\0"
    "player_index,pause\0player_pause_changed(int,bool)\0"
    "player_index,val\0player_cue_changed(int,bool)\0"
    "player_mute_changed(int,bool)\0"
    "player_sync_changed(int,bool)\0"
    "player_loop_changed(int,bool)\0"
    "player_volume_changed(int,int)\0"
    "player_play_speed_changed(int,int)\0"
    "player_position_changed(int,DataJockey::Audio::TimePoint)\0"
    "player_index\0player_audio_file_cleared(int)\0"
    "player_index,percent\0"
    "player_audio_file_load_progress(int,int)\0"
    "player_index,location\0"
    "player_audio_file_changed(int,QString)\0"
    "set_player_pause(int,bool)\0"
    "set_player_cue(int,bool)\0"
    "set_player_mute(int,bool)\0"
    "set_player_sync(int,bool)\0"
    "set_player_loop(int,bool)\0"
    "set_player_volume(int,int)\0"
    "set_player_play_speed(int,int)\0"
    "set_player_position(int,DataJockey::Audio::TimePoint)\0"
    "set_player_start_position(int,DataJockey::Audio::TimePoint)\0"
    "set_player_end_position(int,DataJockey::Audio::TimePoint)\0"
    "set_player_loop_start_position(int,DataJockey::Audio::TimePoint)\0"
    "set_player_loop_end_position(int,DataJockey::Audio::TimePoint)\0"
    "set_player_audio_file(int,QString)\0"
    "set_player_clear_buffers(int)\0val\0"
    "set_master_volume(int)\0"
    "set_master_cue_volume(int)\0enable\0"
    "set_master_cross_fade_enable(bool)\0"
    "set_master_cross_fade_position(int)\0"
    "left,right\0set_master_cross_fade_players(int,int)\0"
};

const QMetaObject DataJockey::Audio::AudioController::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_DataJockey__Audio__AudioController,
      qt_meta_data_DataJockey__Audio__AudioController, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DataJockey::Audio::AudioController::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DataJockey::Audio::AudioController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DataJockey::Audio::AudioController::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DataJockey__Audio__AudioController))
        return static_cast<void*>(const_cast< AudioController*>(this));
    return QObject::qt_metacast(_clname);
}

int DataJockey::Audio::AudioController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: player_pause_changed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 1: player_cue_changed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 2: player_mute_changed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 3: player_sync_changed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 4: player_loop_changed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 5: player_volume_changed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: player_play_speed_changed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 7: player_position_changed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const DataJockey::Audio::TimePoint(*)>(_a[2]))); break;
        case 8: player_audio_file_cleared((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: player_audio_file_load_progress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 10: player_audio_file_changed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 11: set_player_pause((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 12: set_player_cue((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 13: set_player_mute((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 14: set_player_sync((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 15: set_player_loop((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 16: set_player_volume((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 17: set_player_play_speed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 18: set_player_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const DataJockey::Audio::TimePoint(*)>(_a[2]))); break;
        case 19: set_player_start_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const DataJockey::Audio::TimePoint(*)>(_a[2]))); break;
        case 20: set_player_end_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const DataJockey::Audio::TimePoint(*)>(_a[2]))); break;
        case 21: set_player_loop_start_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const DataJockey::Audio::TimePoint(*)>(_a[2]))); break;
        case 22: set_player_loop_end_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const DataJockey::Audio::TimePoint(*)>(_a[2]))); break;
        case 23: set_player_audio_file((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 24: set_player_clear_buffers((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 25: set_master_volume((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 26: set_master_cue_volume((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 27: set_master_cross_fade_enable((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 28: set_master_cross_fade_position((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 29: set_master_cross_fade_players((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
        _id -= 30;
    }
    return _id;
}

// SIGNAL 0
void DataJockey::Audio::AudioController::player_pause_changed(int _t1, bool _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DataJockey::Audio::AudioController::player_cue_changed(int _t1, bool _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DataJockey::Audio::AudioController::player_mute_changed(int _t1, bool _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DataJockey::Audio::AudioController::player_sync_changed(int _t1, bool _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void DataJockey::Audio::AudioController::player_loop_changed(int _t1, bool _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void DataJockey::Audio::AudioController::player_volume_changed(int _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void DataJockey::Audio::AudioController::player_play_speed_changed(int _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void DataJockey::Audio::AudioController::player_position_changed(int _t1, const DataJockey::Audio::TimePoint & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void DataJockey::Audio::AudioController::player_audio_file_cleared(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void DataJockey::Audio::AudioController::player_audio_file_load_progress(int _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void DataJockey::Audio::AudioController::player_audio_file_changed(int _t1, QString _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}
QT_END_MOC_NAMESPACE
