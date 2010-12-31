/****************************************************************************
** Meta object code from reading C++ file 'audioloaderthread.hpp'
**
** Created: Thu Dec 30 21:18:13 2010
**      by: The Qt Meta Object Compiler version 62 (Qt 4.6.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "audioloaderthread.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'audioloaderthread.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.6.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DataJockey__Audio__AudioController__AudioLoaderThread[] = {

 // content:
       4,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      72,   55,   54,   54, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_DataJockey__Audio__AudioController__AudioLoaderThread[] = {
    "DataJockey::Audio::AudioController::AudioLoaderThread\0"
    "\0fileName,percent\0load_progress(QString,int)\0"
};

const QMetaObject DataJockey::Audio::AudioController::AudioLoaderThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_DataJockey__Audio__AudioController__AudioLoaderThread,
      qt_meta_data_DataJockey__Audio__AudioController__AudioLoaderThread, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DataJockey::Audio::AudioController::AudioLoaderThread::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DataJockey::Audio::AudioController::AudioLoaderThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DataJockey::Audio::AudioController::AudioLoaderThread::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DataJockey__Audio__AudioController__AudioLoaderThread))
        return static_cast<void*>(const_cast< AudioLoaderThread*>(this));
    return QThread::qt_metacast(_clname);
}

int DataJockey::Audio::AudioController::AudioLoaderThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: load_progress((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void DataJockey::Audio::AudioController::AudioLoaderThread::load_progress(QString _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
