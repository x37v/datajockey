/****************************************************************************
** Meta object code from reading C++ file 'audiomodel.hpp'
**
** Created: Mon Dec 27 12:22:39 2010
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
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      43,   24,   23,   23, 0x0a,
      87,   70,   23,   23, 0x0a,
     113,   70,   23,   23, 0x0a,
     139,   70,   23,   23, 0x0a,
     165,   70,   23,   23, 0x0a,
     192,   70,   23,   23, 0x0a,
     223,   70,   23,   23, 0x0a,
     258,   70,   23,   23, 0x0a,
     299,   70,   23,   23, 0x0a,
     338,   70,   23,   23, 0x0a,
     384,   70,   23,   23, 0x0a,
     445,  428,   23,   23, 0x0a,
     487,  428,   23,   23, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_DataJockey__AudioModel[] = {
    "DataJockey::AudioModel\0\0player_index,pause\0"
    "set_player_pause(int,bool)\0player_index,val\0"
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
        case 0: set_player_pause((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 1: set_player_mute((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 2: set_player_sync((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 3: set_player_loop((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 4: set_player_volume((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: set_player_play_speed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: set_player_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const TimePoint(*)>(_a[2]))); break;
        case 7: set_player_start_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const TimePoint(*)>(_a[2]))); break;
        case 8: set_player_end_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const TimePoint(*)>(_a[2]))); break;
        case 9: set_player_loop_start_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const TimePoint(*)>(_a[2]))); break;
        case 10: set_player_loop_end_position((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const TimePoint(*)>(_a[2]))); break;
        case 11: set_player_audio_buffer((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< AudioBuffer*(*)>(_a[2]))); break;
        case 12: set_player_beat_buffer((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< BeatBuffer*(*)>(_a[2]))); break;
        default: ;
        }
        _id -= 13;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
