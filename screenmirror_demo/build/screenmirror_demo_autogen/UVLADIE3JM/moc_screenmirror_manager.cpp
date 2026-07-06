/****************************************************************************
** Meta object code from reading C++ file 'screenmirror_manager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/screenmirror_manager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'screenmirror_manager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ScreenMirrorManager_t {
    QByteArrayData data[31];
    char stringdata0[392];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ScreenMirrorManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ScreenMirrorManager_t qt_meta_stringdata_ScreenMirrorManager = {
    {
QT_MOC_LITERAL(0, 0, 19), // "ScreenMirrorManager"
QT_MOC_LITERAL(1, 20, 12), // "stateChanged"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 17), // "deviceNameChanged"
QT_MOC_LITERAL(4, 52, 19), // "audioEnabledChanged"
QT_MOC_LITERAL(5, 72, 18), // "hdcpEnabledChanged"
QT_MOC_LITERAL(6, 91, 19), // "videoQualityChanged"
QT_MOC_LITERAL(7, 111, 14), // "devicesUpdated"
QT_MOC_LITERAL(8, 126, 18), // "QList<QVariantMap>"
QT_MOC_LITERAL(9, 145, 7), // "devices"
QT_MOC_LITERAL(10, 153, 5), // "error"
QT_MOC_LITERAL(11, 159, 7), // "message"
QT_MOC_LITERAL(12, 167, 13), // "statusMessage"
QT_MOC_LITERAL(13, 181, 4), // "init"
QT_MOC_LITERAL(14, 186, 7), // "cleanup"
QT_MOC_LITERAL(15, 194, 14), // "startDiscovery"
QT_MOC_LITERAL(16, 209, 9), // "modeIndex"
QT_MOC_LITERAL(17, 219, 9), // "timeoutMs"
QT_MOC_LITERAL(18, 229, 13), // "stopDiscovery"
QT_MOC_LITERAL(19, 243, 13), // "connectDevice"
QT_MOC_LITERAL(20, 257, 11), // "deviceIndex"
QT_MOC_LITERAL(21, 269, 16), // "disconnectDevice"
QT_MOC_LITERAL(22, 286, 11), // "sendCommand"
QT_MOC_LITERAL(23, 298, 7), // "command"
QT_MOC_LITERAL(24, 306, 14), // "pauseMirroring"
QT_MOC_LITERAL(25, 321, 15), // "resumeMirroring"
QT_MOC_LITERAL(26, 337, 5), // "state"
QT_MOC_LITERAL(27, 343, 10), // "deviceName"
QT_MOC_LITERAL(28, 354, 12), // "audioEnabled"
QT_MOC_LITERAL(29, 367, 11), // "hdcpEnabled"
QT_MOC_LITERAL(30, 379, 12) // "videoQuality"

    },
    "ScreenMirrorManager\0stateChanged\0\0"
    "deviceNameChanged\0audioEnabledChanged\0"
    "hdcpEnabledChanged\0videoQualityChanged\0"
    "devicesUpdated\0QList<QVariantMap>\0"
    "devices\0error\0message\0statusMessage\0"
    "init\0cleanup\0startDiscovery\0modeIndex\0"
    "timeoutMs\0stopDiscovery\0connectDevice\0"
    "deviceIndex\0disconnectDevice\0sendCommand\0"
    "command\0pauseMirroring\0resumeMirroring\0"
    "state\0deviceName\0audioEnabled\0hdcpEnabled\0"
    "videoQuality"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ScreenMirrorManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       5,  140, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  104,    2, 0x06 /* Public */,
       3,    0,  105,    2, 0x06 /* Public */,
       4,    0,  106,    2, 0x06 /* Public */,
       5,    0,  107,    2, 0x06 /* Public */,
       6,    0,  108,    2, 0x06 /* Public */,
       7,    1,  109,    2, 0x06 /* Public */,
      10,    1,  112,    2, 0x06 /* Public */,
      12,    1,  115,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
      13,    0,  118,    2, 0x02 /* Public */,
      14,    0,  119,    2, 0x02 /* Public */,
      15,    2,  120,    2, 0x02 /* Public */,
      15,    1,  125,    2, 0x22 /* Public | MethodCloned */,
      18,    0,  128,    2, 0x02 /* Public */,
      19,    2,  129,    2, 0x02 /* Public */,
      21,    0,  134,    2, 0x02 /* Public */,
      22,    1,  135,    2, 0x02 /* Public */,
      24,    0,  138,    2, 0x02 /* Public */,
      25,    0,  139,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString,   11,

 // methods: parameters
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   16,   17,
    QMetaType::Void, QMetaType::Int,   16,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::Int, QMetaType::Int,   20,   16,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   23,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags
      26, QMetaType::Int, 0x00495001,
      27, QMetaType::QString, 0x00495001,
      28, QMetaType::Bool, 0x00495103,
      29, QMetaType::Bool, 0x00495103,
      30, QMetaType::Int, 0x00495103,

 // properties: notify_signal_id
       0,
       1,
       2,
       3,
       4,

       0        // eod
};

void ScreenMirrorManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ScreenMirrorManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->stateChanged(); break;
        case 1: _t->deviceNameChanged(); break;
        case 2: _t->audioEnabledChanged(); break;
        case 3: _t->hdcpEnabledChanged(); break;
        case 4: _t->videoQualityChanged(); break;
        case 5: _t->devicesUpdated((*reinterpret_cast< QList<QVariantMap>(*)>(_a[1]))); break;
        case 6: _t->error((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 7: _t->statusMessage((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 8: { bool _r = _t->init();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 9: _t->cleanup(); break;
        case 10: _t->startDiscovery((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 11: _t->startDiscovery((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->stopDiscovery(); break;
        case 13: { bool _r = _t->connectDevice((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 14: _t->disconnectDevice(); break;
        case 15: _t->sendCommand((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 16: _t->pauseMirroring(); break;
        case 17: _t->resumeMirroring(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QVariantMap> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ScreenMirrorManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScreenMirrorManager::stateChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ScreenMirrorManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScreenMirrorManager::deviceNameChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ScreenMirrorManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScreenMirrorManager::audioEnabledChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ScreenMirrorManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScreenMirrorManager::hdcpEnabledChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ScreenMirrorManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScreenMirrorManager::videoQualityChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ScreenMirrorManager::*)(QList<QVariantMap> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScreenMirrorManager::devicesUpdated)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (ScreenMirrorManager::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScreenMirrorManager::error)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (ScreenMirrorManager::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScreenMirrorManager::statusMessage)) {
                *result = 7;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<ScreenMirrorManager *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = _t->getState(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->getDeviceName(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->getAudioEnabled(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->getHdcpEnabled(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->getVideoQuality(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<ScreenMirrorManager *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 2: _t->setAudioEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 3: _t->setHdcpEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 4: _t->setVideoQuality(*reinterpret_cast< int*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject ScreenMirrorManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ScreenMirrorManager.data,
    qt_meta_data_ScreenMirrorManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ScreenMirrorManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ScreenMirrorManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ScreenMirrorManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ScreenMirrorManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 5;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void ScreenMirrorManager::stateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ScreenMirrorManager::deviceNameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ScreenMirrorManager::audioEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ScreenMirrorManager::hdcpEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ScreenMirrorManager::videoQualityChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ScreenMirrorManager::devicesUpdated(QList<QVariantMap> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void ScreenMirrorManager::error(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void ScreenMirrorManager::statusMessage(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
