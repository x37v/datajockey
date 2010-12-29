/****************************************************************************
** Meta object code from reading C++ file 'audiomodel.hpp'
**
** Created: Wed Dec 29 09:16:04 2010
**      by: The Qt Meta Object Compiler version 62 (Qt 4.6.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "audiomodel.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'audiomodel.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.6.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DataJockey__AudioModel[] = {

 // content:
       4,       // revision
       0,       // classname
       0,    0, // classinfo
      22,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      45,   24,   23,   23, 0x05,

 // slots: signature, parameters, type, tag, flags
     105,   86,   23,   23, 0x0a,
     149,  132,   23,   23, 0x0a,
     174,  132,   23,   23, 0x0a,
     200,  132,   23,   23, 0x0a,
     226,  132,   23,   23, 0x0a,
     252,  132,   23,   23, 0x0a,
     279,  132,   23,   23, 0x0a,
     310,  132,   23,   23, 0x0a,
     345,  132,   23,   23, 0x0a,
     386,  132,   23,   23, 0x0a,
     425,  132,   23,   23, 0x0a,
     471,  132,   23,   23, 0x0a,
     532,  515,   23,   23, 0x0a,
     574,  515,   23,   23, 0x0a,
     636,  614,   23,   23, 0x0a,
     684,  671,   23,   23, 0x0a,
     718,  714,   23,   23, 0x0a,
     741,  714,   23,   23, 0x0a,
     775,  768,   23,   23, 0x0a,
     810,  714,   23,   23, 0x0a,
     857,  846,   23,   23, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_DataJockey__AudioModel[] = {
    "DataJockey::AudioModel\0\0player_index,percent\0"
    "player_audio_file_load_progress(int,int)\0"
    "player_index,pause\0set_player_pause(int,bool)\0"
    "player_index,val\0set_player_cue(int,bool)\0"
    "set_player_mute(int,bool)\0"
    "set_player_sync(int,bool)\0"
    "set_player_loop(int,bool)\0"
    "set_player_volume(int,int)\0"
    "set_player_play_speed(int,int)\0"
    "set_player_position(int,TimePoint)\0"
    "set_player_start_position(int,TimePoint)\0"
    "set_player_end_position(int,TimePoint)\0"
    "set_player_loop_start_position(int,TimePoint)\0"
    "set_player_loop_end_position(int,TimePoint)\0"
    "player_index,buf\0"
    "set_player_audio_buffer(int,AudioBuffer*)\0"
    "set_player_beat_buffer(int,BeatBuffer*)\0"
    "player_index,location\0"
    "set_player_audio_file(int,QString)\0"
    "player_index\0set_player_clear_buffers(int)\0"
    "val\0set_master_volume(int)\0"
    "set_master_cue_volume(int)\0enable\0"
    "set_master_cross_fade_enable(bool)\0"
    "set_master_cross_fade_position(int)\0"
    "left,right\0set_master_cross_fade_players(int,int)\0"
};

const QMetaObject DataJockey::AudioModel::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_DataJockey__AudioModel,
      qt_meta_data_DataJockey__AudioModel, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DataJockey::AudioModel::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DataJockey::AudioModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DataJockey::AudioModel::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DataJockey__AudioModel))
        return static_cast<void*>(const_cast< AudioModel*>(this));
    return QObject::qt_metacast(_clname);
}

int DataJockey::AudioModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: player_audio_file_load_progress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: set_player_pause((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 2: set_player_cue((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 3: set_player_mute((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 4: set_player_sync((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 5: set_player_loop((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 6: set_player_volume((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 7: set_player_play_speed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 8: set_player_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const TimePoint(*)>(_a[2]))); break;
        case 9: set_player_start_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const TimePoint(*)>(_a[2]))); break;
        case 10: set_player_end_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const TimePoint(*)>(_a[2]))); break;
        case 11: set_player_loop_start_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const TimePoint(*)>(_a[2]))); break;
        case 12: set_player_loop_end_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const TimePoint(*)>(_a[2]))); break;
        case 13: set_player_audio_buffer((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< AudioBuffer*(*)>(_a[2]))); break;
        case 14: set_player_beat_buffer((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< BeatBuffer*(*)>(_a[2]))); break;
        case 15: set_player_audio_file((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 16: set_player_clear_buffers((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 17: set_master_volume((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 18: set_master_cue_volume((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 19: set_master_cross_fade_enable((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 20: set_master_cross_fade_position((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 21: set_master_cross_fade_players((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
        _id -= 22;
    }
    return _id;
}

// SIGNAL 0
void DataJockey::AudioModel::player_audio_file_load_progress(int _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
