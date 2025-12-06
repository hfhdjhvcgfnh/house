/****************************************************************************
** Meta object code from reading C++ file 'chessboardwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../chessboardwidget.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'chessboardwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_ChessBoardWidget_t {
    uint offsetsAndSizes[32];
    char stringdata0[17];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[12];
    char stringdata5[5];
    char stringdata6[19];
    char stringdata7[4];
    char stringdata8[4];
    char stringdata9[9];
    char stringdata10[15];
    char stringdata11[8];
    char stringdata12[6];
    char stringdata13[17];
    char stringdata14[24];
    char stringdata15[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_ChessBoardWidget_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_ChessBoardWidget_t qt_meta_stringdata_ChessBoardWidget = {
    {
        QT_MOC_LITERAL(0, 16),  // "ChessBoardWidget"
        QT_MOC_LITERAL(17, 13),  // "tourCompleted"
        QT_MOC_LITERAL(31, 0),  // ""
        QT_MOC_LITERAL(32, 10),  // "tourFailed"
        QT_MOC_LITERAL(43, 11),  // "stepChanged"
        QT_MOC_LITERAL(55, 4),  // "step"
        QT_MOC_LITERAL(60, 18),  // "hoveredCellChanged"
        QT_MOC_LITERAL(79, 3),  // "row"
        QT_MOC_LITERAL(83, 3),  // "col"
        QT_MOC_LITERAL(87, 8),  // "canClose"
        QT_MOC_LITERAL(96, 14),  // "exportProgress"
        QT_MOC_LITERAL(111, 7),  // "current"
        QT_MOC_LITERAL(119, 5),  // "total"
        QT_MOC_LITERAL(125, 16),  // "onAnimationTimer"
        QT_MOC_LITERAL(142, 23),  // "onAnimationFrameChanged"
        QT_MOC_LITERAL(166, 5)   // "frame"
    },
    "ChessBoardWidget",
    "tourCompleted",
    "",
    "tourFailed",
    "stepChanged",
    "step",
    "hoveredCellChanged",
    "row",
    "col",
    "canClose",
    "exportProgress",
    "current",
    "total",
    "onAnimationTimer",
    "onAnimationFrameChanged",
    "frame"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_ChessBoardWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   56,    2, 0x06,    1 /* Public */,
       3,    0,   57,    2, 0x06,    2 /* Public */,
       4,    1,   58,    2, 0x06,    3 /* Public */,
       6,    3,   61,    2, 0x06,    5 /* Public */,
      10,    2,   68,    2, 0x06,    9 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      13,    0,   73,    2, 0x08,   12 /* Private */,
      14,    1,   74,    2, 0x08,   13 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Bool,    7,    8,    9,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   11,   12,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   15,

       0        // eod
};

Q_CONSTINIT const QMetaObject ChessBoardWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ChessBoardWidget.offsetsAndSizes,
    qt_meta_data_ChessBoardWidget,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_ChessBoardWidget_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ChessBoardWidget, std::true_type>,
        // method 'tourCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'tourFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stepChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'hoveredCellChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'exportProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onAnimationTimer'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAnimationFrameChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void ChessBoardWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChessBoardWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->tourCompleted(); break;
        case 1: _t->tourFailed(); break;
        case 2: _t->stepChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->hoveredCellChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 4: _t->exportProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 5: _t->onAnimationTimer(); break;
        case 6: _t->onAnimationFrameChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChessBoardWidget::*)();
            if (_t _q_method = &ChessBoardWidget::tourCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ChessBoardWidget::*)();
            if (_t _q_method = &ChessBoardWidget::tourFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ChessBoardWidget::*)(int );
            if (_t _q_method = &ChessBoardWidget::stepChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ChessBoardWidget::*)(int , int , bool );
            if (_t _q_method = &ChessBoardWidget::hoveredCellChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ChessBoardWidget::*)(int , int );
            if (_t _q_method = &ChessBoardWidget::exportProgress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *ChessBoardWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChessBoardWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ChessBoardWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ChessBoardWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void ChessBoardWidget::tourCompleted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ChessBoardWidget::tourFailed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ChessBoardWidget::stepChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ChessBoardWidget::hoveredCellChanged(int _t1, int _t2, bool _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ChessBoardWidget::exportProgress(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
