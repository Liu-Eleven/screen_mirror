/****************************************************************************
** Meta object code from reading C++ file 'wifi_manager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/wifi_manager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'wifi_manager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_WiFiManager_t {
    QByteArrayData data[25];
    char stringdata0[305];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_WiFiManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_WiFiManager_t qt_meta_stringdata_WiFiManager = {
    {
QT_MOC_LITERAL(0, 0, 11), // "WiFiManager"
QT_MOC_LITERAL(1, 12, 14), // "enabledChanged"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 20), // "connectedSSIDChanged"
QT_MOC_LITERAL(4, 49, 15), // "scanningChanged"
QT_MOC_LITERAL(5, 65, 15), // "networksUpdated"
QT_MOC_LITERAL(6, 81, 18), // "QList<QVariantMap>"
QT_MOC_LITERAL(7, 100, 8), // "networks"
QT_MOC_LITERAL(8, 109, 5), // "error"
QT_MOC_LITERAL(9, 115, 7), // "message"
QT_MOC_LITERAL(10, 123, 11), // "scanStarted"
QT_MOC_LITERAL(11, 135, 12), // "scanFinished"
QT_MOC_LITERAL(12, 148, 14), // "connectSuccess"
QT_MOC_LITERAL(13, 163, 4), // "ssid"
QT_MOC_LITERAL(14, 168, 13), // "statusMessage"
QT_MOC_LITERAL(15, 182, 13), // "onScanTimeout"
QT_MOC_LITERAL(16, 196, 9), // "startWiFi"
QT_MOC_LITERAL(17, 206, 8), // "stopWiFi"
QT_MOC_LITERAL(18, 215, 12), // "scanNetworks"
QT_MOC_LITERAL(19, 228, 14), // "connectNetwork"
QT_MOC_LITERAL(20, 243, 8), // "password"
QT_MOC_LITERAL(21, 252, 17), // "disconnectNetwork"
QT_MOC_LITERAL(22, 270, 9), // "isEnabled"
QT_MOC_LITERAL(23, 280, 13), // "connectedSSID"
QT_MOC_LITERAL(24, 294, 10) // "isScanning"

    },
    "WiFiManager\0enabledChanged\0\0"
    "connectedSSIDChanged\0scanningChanged\0"
    "networksUpdated\0QList<QVariantMap>\0"
    "networks\0error\0message\0scanStarted\0"
    "scanFinished\0connectSuccess\0ssid\0"
    "statusMessage\0onScanTimeout\0startWiFi\0"
    "stopWiFi\0scanNetworks\0connectNetwork\0"
    "password\0disconnectNetwork\0isEnabled\0"
    "connectedSSID\0isScanning"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_WiFiManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       3,  116, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   89,    2, 0x06 /* Public */,
       3,    0,   90,    2, 0x06 /* Public */,
       4,    0,   91,    2, 0x06 /* Public */,
       5,    1,   92,    2, 0x06 /* Public */,
       8,    1,   95,    2, 0x06 /* Public */,
      10,    0,   98,    2, 0x06 /* Public */,
      11,    0,   99,    2, 0x06 /* Public */,
      12,    1,  100,    2, 0x06 /* Public */,
      14,    1,  103,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      15,    0,  106,    2, 0x08 /* Private */,

 // methods: name, argc, parameters, tag, flags
      16,    0,  107,    2, 0x02 /* Public */,
      17,    0,  108,    2, 0x02 /* Public */,
      18,    0,  109,    2, 0x02 /* Public */,
      19,    2,  110,    2, 0x02 /* Public */,
      21,    0,  115,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void, QMetaType::QString,    9,

 // slots: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString,   13,   20,
    QMetaType::Void,

 // properties: name, type, flags
      22, QMetaType::Bool, 0x00495001,
      23, QMetaType::QString, 0x00495001,
      24, QMetaType::Bool, 0x00495001,

 // properties: notify_signal_id
       0,
       1,
       2,

       0        // eod
};

void WiFiManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WiFiManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->enabledChanged(); break;
        case 1: _t->connectedSSIDChanged(); break;
        case 2: _t->scanningChanged(); break;
        case 3: _t->networksUpdated((*reinterpret_cast< QList<QVariantMap>(*)>(_a[1]))); break;
        case 4: _t->error((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 5: _t->scanStarted(); break;
        case 6: _t->scanFinished(); break;
        case 7: _t->connectSuccess((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 8: _t->statusMessage((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 9: _t->onScanTimeout(); break;
        case 10: { bool _r = _t->startWiFi();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 11: { bool _r = _t->stopWiFi();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 12: _t->scanNetworks(); break;
        case 13: { bool _r = _t->connectNetwork((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 14: _t->disconnectNetwork(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 3:
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
            using _t = void (WiFiManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WiFiManager::enabledChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (WiFiManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WiFiManager::connectedSSIDChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (WiFiManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WiFiManager::scanningChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (WiFiManager::*)(QList<QVariantMap> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WiFiManager::networksUpdated)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (WiFiManager::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WiFiManager::error)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (WiFiManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WiFiManager::scanStarted)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (WiFiManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WiFiManager::scanFinished)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (WiFiManager::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WiFiManager::connectSuccess)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (WiFiManager::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WiFiManager::statusMessage)) {
                *result = 8;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<WiFiManager *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isEnabled(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->connectedSSID(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->isScanning(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject WiFiManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_WiFiManager.data,
    qt_meta_data_WiFiManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *WiFiManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WiFiManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_WiFiManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int WiFiManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void WiFiManager::enabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void WiFiManager::connectedSSIDChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void WiFiManager::scanningChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void WiFiManager::networksUpdated(QList<QVariantMap> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void WiFiManager::error(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void WiFiManager::scanStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void WiFiManager::scanFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void WiFiManager::connectSuccess(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void WiFiManager::statusMessage(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
