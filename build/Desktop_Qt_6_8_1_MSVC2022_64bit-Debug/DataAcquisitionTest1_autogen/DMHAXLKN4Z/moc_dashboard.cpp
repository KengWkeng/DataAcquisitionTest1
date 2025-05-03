/****************************************************************************
** Meta object code from reading C++ file 'dashboard.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../plot/dashboard.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dashboard.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9DashboardE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN9DashboardE = QtMocHelpers::stringData(
    "Dashboard",
    "valueChanged",
    "",
    "value",
    "rangeChanged",
    "labelChanged",
    "label",
    "unitChanged",
    "unit",
    "precisionChanged",
    "precision",
    "styleChanged",
    "animationChanged",
    "enabled",
    "updateAnimation",
    "configure",
    "minRange",
    "maxRange",
    "minValue",
    "maxValue",
    "scaleColor",
    "pointerStyle",
    "PointerStyle",
    "pointerColor",
    "foregroundColor",
    "textColor",
    "animationEnabled"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN9DashboardE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
      12,   97, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   68,    2, 0x06,   13 /* Public */,
       4,    0,   71,    2, 0x06,   15 /* Public */,
       5,    1,   72,    2, 0x06,   16 /* Public */,
       7,    1,   75,    2, 0x06,   18 /* Public */,
       9,    1,   78,    2, 0x06,   20 /* Public */,
      11,    0,   81,    2, 0x06,   22 /* Public */,
      12,    1,   82,    2, 0x06,   23 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      14,    0,   85,    2, 0x08,   25 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      15,    5,   86,    2, 0x02,   26 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Double,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   13,

 // slots: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::Int, QMetaType::Double, QMetaType::Double,    6,    8,   10,   16,   17,

 // properties: name, type, flags, notifyId, revision
       3, QMetaType::Double, 0x00015103, uint(0), 0,
      18, QMetaType::Double, 0x00015103, uint(1), 0,
      19, QMetaType::Double, 0x00015103, uint(1), 0,
      10, QMetaType::Int, 0x00015103, uint(4), 0,
       8, QMetaType::QString, 0x00015103, uint(3), 0,
       6, QMetaType::QString, 0x00015103, uint(2), 0,
      20, QMetaType::QColor, 0x00015103, uint(5), 0,
      21, 0x80000000 | 22, 0x0001510b, uint(5), 0,
      23, QMetaType::QColor, 0x00015103, uint(5), 0,
      24, QMetaType::QColor, 0x00015103, uint(5), 0,
      25, QMetaType::QColor, 0x00015103, uint(5), 0,
      26, QMetaType::Bool, 0x00015103, uint(6), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject Dashboard::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN9DashboardE.offsetsAndSizes,
    qt_meta_data_ZN9DashboardE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN9DashboardE_t,
        // property 'value'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'minValue'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'maxValue'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'precision'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'unit'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'label'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'scaleColor'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'pointerStyle'
        QtPrivate::TypeAndForceComplete<PointerStyle, std::true_type>,
        // property 'pointerColor'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'foregroundColor'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'textColor'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'animationEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Dashboard, std::true_type>,
        // method 'valueChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'rangeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'labelChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'unitChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'precisionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'styleChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'animationChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'updateAnimation'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'configure'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>
    >,
    nullptr
} };

void Dashboard::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Dashboard *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 1: _t->rangeChanged(); break;
        case 2: _t->labelChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->unitChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->precisionChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->styleChanged(); break;
        case 6: _t->animationChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->updateAnimation(); break;
        case 8: _t->configure((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[5]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (Dashboard::*)(double );
            if (_q_method_type _q_method = &Dashboard::valueChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (Dashboard::*)();
            if (_q_method_type _q_method = &Dashboard::rangeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (Dashboard::*)(const QString & );
            if (_q_method_type _q_method = &Dashboard::labelChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (Dashboard::*)(const QString & );
            if (_q_method_type _q_method = &Dashboard::unitChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (Dashboard::*)(int );
            if (_q_method_type _q_method = &Dashboard::precisionChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (Dashboard::*)();
            if (_q_method_type _q_method = &Dashboard::styleChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (Dashboard::*)(bool );
            if (_q_method_type _q_method = &Dashboard::animationChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< double*>(_v) = _t->value(); break;
        case 1: *reinterpret_cast< double*>(_v) = _t->minValue(); break;
        case 2: *reinterpret_cast< double*>(_v) = _t->maxValue(); break;
        case 3: *reinterpret_cast< int*>(_v) = _t->precision(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->unit(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->label(); break;
        case 6: *reinterpret_cast< QColor*>(_v) = _t->scaleColor(); break;
        case 7: *reinterpret_cast< PointerStyle*>(_v) = _t->pointerStyle(); break;
        case 8: *reinterpret_cast< QColor*>(_v) = _t->pointerColor(); break;
        case 9: *reinterpret_cast< QColor*>(_v) = _t->foregroundColor(); break;
        case 10: *reinterpret_cast< QColor*>(_v) = _t->textColor(); break;
        case 11: *reinterpret_cast< bool*>(_v) = _t->isAnimationEnabled(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setValue(*reinterpret_cast< double*>(_v)); break;
        case 1: _t->setMinValue(*reinterpret_cast< double*>(_v)); break;
        case 2: _t->setMaxValue(*reinterpret_cast< double*>(_v)); break;
        case 3: _t->setPrecision(*reinterpret_cast< int*>(_v)); break;
        case 4: _t->setUnit(*reinterpret_cast< QString*>(_v)); break;
        case 5: _t->setLabel(*reinterpret_cast< QString*>(_v)); break;
        case 6: _t->setScaleColor(*reinterpret_cast< QColor*>(_v)); break;
        case 7: _t->setPointerStyle(*reinterpret_cast< PointerStyle*>(_v)); break;
        case 8: _t->setPointerColor(*reinterpret_cast< QColor*>(_v)); break;
        case 9: _t->setForegroundColor(*reinterpret_cast< QColor*>(_v)); break;
        case 10: _t->setTextColor(*reinterpret_cast< QColor*>(_v)); break;
        case 11: _t->setAnimationEnabled(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *Dashboard::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Dashboard::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN9DashboardE.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int Dashboard::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void Dashboard::valueChanged(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Dashboard::rangeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Dashboard::labelChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Dashboard::unitChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Dashboard::precisionChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Dashboard::styleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void Dashboard::animationChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
