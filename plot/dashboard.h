#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QPaintEvent>
#include <QTimer>
#include <QMouseEvent>
#include <QMap>
#include <QVariant>
#include <QColor>
#include <QVector>
#include <QMetaType>
#include <QPixmap>
#include <QFont>
// #include "dashboardcalculator.h" // 引入计算线程类

// 为QVector<int>类型声明元类型，以支持属性系统
Q_DECLARE_METATYPE(QVector<int>)

// 前向声明
// class QDialog;
// class QGroupBox;
// class QPushButton;

// 定义指针样式枚举
enum PointerStyle {
    PointerStyle_Circle = 0,
    PointerStyle_Indicator = 1,
    PointerStyle_Triangle = 2,
    PointerStyle_Custom = 3
};

/**
 * @class Dashboard
 * @brief 自定义仪表盘控件
 *
 * 仪表盘使用Qt的角度坐标系（0度在右侧，逆时针为正方向）
 * 刻度起始角度为130度
 * 刻度结束角度为50度
 * 刻度总范围为280度
 */
class Dashboard : public QWidget
{
    Q_OBJECT

    // 使用Qt属性系统
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minValue READ minValue WRITE setMinValue NOTIFY rangeChanged)
    Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue NOTIFY rangeChanged)
    Q_PROPERTY(int precision READ precision WRITE setPrecision NOTIFY precisionChanged)
    Q_PROPERTY(QString unit READ unit WRITE setUnit NOTIFY unitChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QColor scaleColor READ scaleColor WRITE setScaleColor NOTIFY styleChanged)
    Q_PROPERTY(PointerStyle pointerStyle READ pointerStyle WRITE setPointerStyle NOTIFY styleChanged)
    Q_PROPERTY(QColor pointerColor READ pointerColor WRITE setPointerColor NOTIFY styleChanged)
    Q_PROPERTY(QColor foregroundColor READ foregroundColor WRITE setForegroundColor NOTIFY styleChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY styleChanged)
    Q_PROPERTY(bool animationEnabled READ isAnimationEnabled WRITE setAnimationEnabled NOTIFY animationChanged)

public:
    explicit Dashboard(QWidget *parent = nullptr);
    ~Dashboard();

    // Getter/Setter
    double value() const;
    double minValue() const;
    double maxValue() const;
    int precision() const;
    QString unit() const;
    QString label() const;
    QColor scaleColor() const;
    QColor pointerColor() const;
    PointerStyle pointerStyle() const;
    QColor foregroundColor() const;
    QColor textColor() const;
    bool isAnimationEnabled() const;

    void setValue(double value);
    void setMinValue(double minValue);
    void setMaxValue(double maxValue);
    void setRange(double minValue, double maxValue);
    void setPrecision(int precision);
    void setUnit(const QString &unit);
    void setLabel(const QString &label);
    void setScaleColor(const QColor &color);
    void setPointerStyle(PointerStyle style);
    void setPointerColor(const QColor &color);
    void setForegroundColor(const QColor &color);
    void setTextColor(const QColor &color);
    void setAnimationEnabled(bool enabled);

    // 添加公式计算功能 - 保留公共接口，但修改内部实现使用计算线程
    double calculateFormula(const QString &formula, const QMap<QString, double> &variables);

    // 设置计算线程
    // void setCalculator(DashboardCalculator *calculator);

    // 设置初始化状态
    void setInitializationStatus(bool initialized) { m_initialized = initialized; }

    // 新增 configure 方法
    Q_INVOKABLE void configure(const QString &label, const QString &unit, int precision, double minRange, double maxRange);

public slots:
    // 设置用于公式计算的变量值
    // void setVariableValues(const QMap<QString, double> &variables);
    // 接收来自计算线程的计算结果
    // void onCalculationResult(const QString &dashboardName, double result);
    // 接收来自计算线程的计算错误
    // void onCalculationError(const QString &dashboardName, const QString &errorMessage);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    // 移除 mouseDoubleClickEvent (除非需要配置功能)
    // void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void updateAnimation();

signals:
    void valueChanged(double value);
    void rangeChanged();
    void labelChanged(const QString &label);
    void unitChanged(const QString &unit);
    void precisionChanged(int precision);
    void styleChanged();
    void animationChanged(bool enabled);

private:
    // 移除不再需要的工具方法
    // void updateLabel();
    // void drawThemeBackground(...);
    // double evaluateExpression(...);
    // int precedence(...);
    // bool isOperator(...);
    // double applyOperator(...);

    // 移除计算和变量相关的成员
    // QMap<QString, double> m_lastVariableValues;
    // bool m_hasValidResult;
    // DashboardCalculator *m_calculator;
    // double m_lastCalculatedValue;
    // bool m_lastCalculationSucceeded;
    // QString m_variableName;
    // QString m_formula;
    // bool m_isCustomVariable;
    // QMap<QString, double> m_variables;
    // bool m_initialized;

    // 核心数值属性
    double m_value = 0.0;           // 目标值
    double m_currentValue = 0.0;    // 动画当前值
    double m_minValue = 0.0;
    double m_maxValue = 100.0;
    int m_precision = 0;
    QString m_unit = "";
    QString m_label = "Dashboard"; // 替换 m_title

    // 视觉样式属性
    QColor m_backgroundColor = Qt::white;
    QColor m_foregroundColor = QColor(50, 50, 50); // 外环、指针中心等颜色
    QColor m_scaleColor = Qt::black;
    QColor m_textColor = Qt::black;       // 刻度数字、单位、标签颜色
    QColor m_pointerColor = QColor(200, 0, 0);
    PointerStyle m_pointerStyle = PointerStyle_Indicator; // 默认指针样式

    // 动画相关
    bool m_animationEnabled = true; // 替换 m_animation
    QTimer *m_animationTimer;    // 替换 m_timer
    double m_animationStep = 1.0; // 固定的步长或基于范围的计算

    // 刻度相关
    int m_scaleMinorTicks = 5; // 小刻度数量 (每大格)
    int m_scaleMajorTicks = 10; // 大刻度数量 (总数)
    double m_startAngle; // 刻度起始角度 (度)
    double m_endAngle;   // 刻度结束角度 (度)
    double m_totalAngleSpan; // 总角度范围 (计算得出或设定)

    // 缓存
    QPixmap m_staticCache;
    bool m_cacheDirty = true;

    // 字体
    QFont m_scaleFont;

    // 内部绘图方法
    void updateStaticCache();
    void drawBackground(QPainter *painter);
    void drawScale(QPainter *painter);
    void drawScaleLabels(QPainter *painter);
    void drawLabel(QPainter *painter);
    void drawUnit(QPainter *painter);
    void drawValueDisplay(QPainter *painter);
    void drawInfoBar(QPainter *painter);
    void drawPointer(QPainter *painter);
    void drawCenterDisc(QPainter *painter);

    // 初始化状态
    bool m_initialized = false; // 默认为未初始化
};

#endif // DASHBOARD_H