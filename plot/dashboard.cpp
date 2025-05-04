#include "dashboard.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QDebug>
#include <QtMath>
#include <QMouseEvent>
#include <QTimer>
#include <QMetaObject>
#include <QVariant>
#include <cmath>
#include <QApplication> // For font

// --- 常量定义 ---
constexpr double DEFAULT_START_ANGLE = 225.0;
constexpr double DEFAULT_END_ANGLE = 315.0;
constexpr double DEFAULT_TOTAL_ANGLE_SPAN = 270.0; // 360  - (235 - 315) if wraps around, otherwise 235 + (360 - 315) = 235 + 45 = 280? No, it's 235 - 315 = -80, so 360-80 = 280? Let's recheck. Qt angles: 0 right, 90 up. 235 is bottom-left quadrant. 315 is top-right quadrant. Span is 235 + (360-315) = 235+45=280 WRONG. Span is from 235 counter-clockwise to 315. 235->180 (45) ->270 (90) -> 360 (90) -> 45 (45). 45+90+90+45 = 270. Yes.

Dashboard::Dashboard(QWidget *parent) : QWidget(parent),
    m_value(0.0),
    m_currentValue(0.0),
    m_minValue(0.0),
    m_maxValue(100.0),
    m_precision(0),
    m_unit(""),
    m_label("Dashboard"),
    m_backgroundColor(Qt::white),
    m_foregroundColor(QColor(50, 50, 50)), // Darker foreground
    m_scaleColor(Qt::black),
    m_textColor(Qt::black),
    m_pointerColor(QColor(200, 0, 0)), // Darker red pointer
    m_pointerStyle(PointerStyle_Indicator),
    m_animationEnabled(true),
    m_animationTimer(new QTimer(this)),
    m_animationStep(1.0), // Initial step, might adjust based on range
    m_scaleMinorTicks(5),
    m_scaleMajorTicks(10),
    m_startAngle(DEFAULT_START_ANGLE),
    m_endAngle(DEFAULT_END_ANGLE),
    m_totalAngleSpan(DEFAULT_TOTAL_ANGLE_SPAN),
    m_cacheDirty(true)
{
    // Fonts
    m_scaleFont = QApplication::font();

    // Timer setup
    m_animationTimer->setInterval(20); // Animation refresh rate
    connect(m_animationTimer, &QTimer::timeout, this, &Dashboard::updateAnimation);

    setMinimumSize(200, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

Dashboard::~Dashboard()
{
    // Timer is child of 'this', automatically deleted
}

// --- Getters Implementation ---
double Dashboard::value() const { return m_value; }
double Dashboard::minValue() const { return m_minValue; }
double Dashboard::maxValue() const { return m_maxValue; }
int Dashboard::precision() const { return m_precision; }
QString Dashboard::unit() const { return m_unit; }
QString Dashboard::label() const { return m_label; }
QColor Dashboard::scaleColor() const { return m_scaleColor; }
QColor Dashboard::pointerColor() const { return m_pointerColor; }
PointerStyle Dashboard::pointerStyle() const { return m_pointerStyle; }
QColor Dashboard::foregroundColor() const { return m_foregroundColor; }
QColor Dashboard::textColor() const { return m_textColor; }
bool Dashboard::isAnimationEnabled() const { return m_animationEnabled; }

// --- Configure Method ---
void Dashboard::configure(const QString &label, const QString &unit, int precision, double minRange, double maxRange)
{
    bool rangeChangedFlag = false;
    bool styleChangedFlag = false;

    if (m_label != label) {
        m_label = label;
        styleChangedFlag = true; // Label change requires cache update
        emit labelChanged(m_label);
    }
    if (m_unit != unit) {
        m_unit = unit;
        styleChangedFlag = true; // Unit change requires cache update
        emit unitChanged(m_unit);
    }
    if (m_precision != precision && precision >= 0) {
        m_precision = precision;
        styleChangedFlag = true; // Precision affects value display (in cache)
        emit precisionChanged(m_precision);
    }

    if (minRange < maxRange) {
        if (m_minValue != minRange) {
            m_minValue = minRange;
            rangeChangedFlag = true;
        }
        if (m_maxValue != maxRange) {
            m_maxValue = maxRange;
            rangeChangedFlag = true;
        }
        if (rangeChangedFlag) {
            styleChangedFlag = true; // Range change affects scale (in cache)
            emit rangeChanged();
        }
    }

    if (styleChangedFlag) {
        m_cacheDirty = true;
        // Re-clamp and update value display immediately if needed
        setValue(m_value);
        update(); // Trigger repaint
    }
}

// --- Setters Implementation ---
void Dashboard::setValue(double value)
{
    double clampedValue = std::clamp(value, m_minValue, m_maxValue);
    if (m_value != clampedValue) {
        m_value = clampedValue;
        emit valueChanged(m_value);

        if (m_animationEnabled && m_value != m_currentValue) {
            // Adjust step based on difference?
            double diff = qAbs(m_value - m_currentValue);
            // Simple fixed step for now, could be more complex
            m_animationStep = (m_maxValue - m_minValue) / 100.0; // e.g., 1% of range per step
            m_animationStep = qMax(0.1, m_animationStep); // Ensure minimum step

            m_animationTimer->start();
        } else {
            m_currentValue = m_value;
             if (m_animationTimer->isActive()) m_animationTimer->stop();
            update(); // Update display immediately if no animation
        }
    }
     else if (!m_animationEnabled && m_currentValue != m_value) {
         // Ensure currentValue matches value if animation is disabled
         m_currentValue = m_value;
         update();
     }
}

void Dashboard::setMinValue(double minValue)
{
    configure(m_label, m_unit, m_precision, minValue, m_maxValue);
}

void Dashboard::setMaxValue(double maxValue)
{
    configure(m_label, m_unit, m_precision, m_minValue, maxValue);
}

void Dashboard::setRange(double minValue, double maxValue)
{
    configure(m_label, m_unit, m_precision, minValue, maxValue);
}

void Dashboard::setPrecision(int precision)
{
    configure(m_label, m_unit, precision, m_minValue, m_maxValue);
}

void Dashboard::setUnit(const QString &unit)
{
    configure(m_label, unit, m_precision, m_minValue, m_maxValue);
}

void Dashboard::setLabel(const QString &label)
{
    configure(label, m_unit, m_precision, m_minValue, m_maxValue);
}

void Dashboard::setScaleColor(const QColor &color)
{
    if (m_scaleColor != color) {
        m_scaleColor = color;
        m_cacheDirty = true;
        update();
        emit styleChanged();
    }
}

void Dashboard::setPointerStyle(PointerStyle style)
{
    if (m_pointerStyle != style) {
        m_pointerStyle = style;
        // Pointer is dynamic, no cache update needed unless center disc changes
        update();
        emit styleChanged();
    }
}

void Dashboard::setPointerColor(const QColor &color)
{
    if (m_pointerColor != color) {
        m_pointerColor = color;
        // Pointer is dynamic, no cache update needed
        update();
        emit styleChanged();
    }
}

void Dashboard::setForegroundColor(const QColor &color)
{
    if (m_foregroundColor != color) {
        m_foregroundColor = color;
        m_cacheDirty = true;
        update();
        emit styleChanged();
    }
}

void Dashboard::setTextColor(const QColor &color)
{
    if (m_textColor != color) {
        m_textColor = color;
        m_cacheDirty = true;
        update();
        emit styleChanged();
    }
}

void Dashboard::setAnimationEnabled(bool enabled)
{
    if (m_animationEnabled != enabled) {
        m_animationEnabled = enabled;
        if (!enabled) { // If disabling animation, jump to final value
            if (m_animationTimer->isActive()) m_animationTimer->stop();
            m_currentValue = m_value;
            update();
        }
        emit animationChanged(enabled);
    }
}

// --- Event Handlers ---
void Dashboard::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_cacheDirty = true;
    update(); // Trigger repaint which will update cache if dirty
    QWidget::resizeEvent(event); // Call base class implementation
}

void Dashboard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Update cache if necessary (size changed, style changed)
    if (m_cacheDirty || m_staticCache.isNull() || m_staticCache.size() != size() * devicePixelRatioF()) {
        updateStaticCache();
    }

    // Draw static background from cache
    painter.drawPixmap(rect(), m_staticCache);

    // Draw dynamic elements
    drawValueDisplay(&painter); // Draw current value
    drawPointer(&painter);
    drawCenterDisc(&painter); // Center disc might overlap pointer base
}

// --- Animation Slot ---
void Dashboard::updateAnimation()
{
    if (!qFuzzyCompare(m_currentValue, m_value)) {
        double step = m_animationStep;
        if (m_currentValue < m_value) {
            m_currentValue += step;
            if (m_currentValue >= m_value) {
                m_currentValue = m_value;
                m_animationTimer->stop();
            }
        } else {
            m_currentValue -= step;
            if (m_currentValue <= m_value) {
                m_currentValue = m_value;
                m_animationTimer->stop();
            }
        }
        update(); // Trigger repaint for pointer movement
    } else {
        m_animationTimer->stop();
    }
}

// --- Static Cache Update ---
void Dashboard::updateStaticCache()
{
    m_staticCache = QPixmap(size() * devicePixelRatioF());
    m_staticCache.setDevicePixelRatio(devicePixelRatioF());
    m_staticCache.fill(Qt::transparent);

    QPainter cachePainter(&m_staticCache);
    cachePainter.setRenderHint(QPainter::Antialiasing);

    drawBackground(&cachePainter);
    drawScale(&cachePainter);
    drawScaleLabels(&cachePainter);
    // 不再在静态缓存中绘制标签和单位，改为在动态部分绘制

    m_cacheDirty = false;
}

// --- Drawing Methods ---

void Dashboard::drawBackground(QPainter *painter)
{
    painter->save();
    painter->setBrush(m_backgroundColor);
    painter->setPen(Qt::NoPen);

    int side = qMin(width(), height() - 30); // 减少高度，为底部信息栏留出空间
    QPoint center = QPoint(width() / 2, (height() - 30) / 2); // 调整中心点位置

    painter->drawEllipse(center, side / 2 - 1, side / 2 - 1);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(m_foregroundColor, 2));
    painter->drawEllipse(center, side / 2 - 2, side / 2 - 2);
    painter->restore();
}

void Dashboard::drawScale(QPainter *painter)
{
    painter->save();
    painter->setPen(m_scaleColor);
    int side = qMin(width(), height() - 30); // 减少高度，为底部信息栏留出空间
    int radius = side / 2 - 10;

    // 调整中心点位置
    painter->translate(QPoint(width() / 2, (height() - 30) / 2));

    double range = m_maxValue - m_minValue;
    if (qFuzzyIsNull(range)) range = 1.0;

    int totalTicks = qMax(1, m_scaleMajorTicks * m_scaleMinorTicks);
    double angleStep = m_totalAngleSpan / totalTicks;

    painter->setPen(QPen(m_scaleColor, 2));
    for (int i = 0; i <= m_scaleMajorTicks; ++i) {
        double angle = m_startAngle - i * (m_totalAngleSpan / m_scaleMajorTicks);
        painter->save();
        painter->rotate(-angle);
        painter->drawLine(radius - 8, 0, radius, 0);
        painter->restore();
    }

    painter->setPen(QPen(m_scaleColor, 1));
     for (int i = 0; i <= totalTicks; ++i) {
         if (i % m_scaleMinorTicks == 0 && m_scaleMinorTicks > 0) continue;
        double angle = m_startAngle - i * angleStep;
         painter->save();
        painter->rotate(-angle);
        painter->drawLine(radius - 4, 0, radius, 0);
         painter->restore();
    }

    painter->restore();
}

void Dashboard::drawScaleLabels(QPainter *painter)
{
    painter->save();
    painter->setPen(m_textColor);
    painter->setFont(m_scaleFont);

    int side = qMin(width(), height() - 30); // 减少高度，为底部信息栏留出空间
    int radius = side / 2 - 35;
    QFontMetrics fm = painter->fontMetrics();

    // 调整中心点位置
    painter->translate(QPoint(width() / 2, (height() - 30) / 2));

    double range = m_maxValue - m_minValue;
    if (qFuzzyIsNull(range)) range = 1.0;

    for (int i = 0; i <= m_scaleMajorTicks; ++i) {
        double value = m_minValue + i * (range / m_scaleMajorTicks);
        QString valueStr = QString::number(std::round(value));
        double angle = m_startAngle - i * (m_totalAngleSpan / m_scaleMajorTicks);
        double angleRad = qDegreesToRadians(angle);

        int textWidth = fm.horizontalAdvance(valueStr);
        int textHeight = fm.height();

        int x = static_cast<int>(radius * qCos(angleRad) - textWidth / 2.0);
        int y = static_cast<int>(-radius * qSin(angleRad) + textHeight / 4.0);

        painter->drawText(x, y, valueStr);
    }

    painter->restore();
}

void Dashboard::drawPointer(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_pointerColor);

    // 调整中心点位置
    painter->translate(QPoint(width() / 2, (height() - 30) / 2));

    double range = m_maxValue - m_minValue;
    if (qFuzzyIsNull(range)) range = 1.0;

    double valueRatio = (m_currentValue - m_minValue) / range;
    double angle = m_startAngle - valueRatio * m_totalAngleSpan;

    painter->rotate(-angle);

    int side = qMin(width(), height() - 30); // 减少高度，为底部信息栏留出空间
    int pointerLength = side / 2 - 20;
    int pointerWidth = 6;

    if (m_pointerStyle == PointerStyle_Indicator || m_pointerStyle == PointerStyle_Triangle) {
         QPolygon pointerPoly;
         pointerPoly << QPoint(0, -pointerWidth/2) << QPoint(pointerLength, 0) << QPoint(0, pointerWidth/2);
         painter->drawPolygon(pointerPoly);
    }
    else {
         painter->setPen(QPen(m_pointerColor, 2));
         painter->drawLine(0, 0, pointerLength - 5, 0);
    }

    painter->restore();
}

void Dashboard::drawCenterDisc(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_foregroundColor);
    int radius = 8;
    // 调整中心点位置
    QPoint center = QPoint(width() / 2, (height() - 30) / 2);
    painter->drawEllipse(center, radius, radius);
    painter->restore();
}

void Dashboard::drawLabel(QPainter *painter)
{
    // 不再单独绘制标签，改为在drawInfoBar中统一绘制
}

void Dashboard::drawUnit(QPainter *painter)
{
    // 不再单独绘制单位，改为在drawInfoBar中统一绘制
}

void Dashboard::drawValueDisplay(QPainter *painter)
{
    // 不再单独绘制数值，改为在drawInfoBar中统一绘制
    drawInfoBar(painter);
}

void Dashboard::drawInfoBar(QPainter *painter)
{
    painter->save();
    painter->setPen(m_textColor);

    // 设置字体
    QFont labelFont = painter->font();
    labelFont.setPointSize(10);
    painter->setFont(labelFont);

    // 格式化当前值
    QString valueText = QString::number(m_currentValue, 'f', m_precision);

    // 计算文本宽度
    QFontMetrics fm = painter->fontMetrics();
    int labelWidth = m_label.isEmpty() ? 0 : fm.horizontalAdvance(m_label);
    int valueWidth = fm.horizontalAdvance(valueText);
    int unitWidth = m_unit.isEmpty() ? 0 : fm.horizontalAdvance(m_unit);

    // 计算间距
    const int spacing = 10; // 文本之间的间距
    int totalWidth = labelWidth + valueWidth + unitWidth;
    if (!m_label.isEmpty()) totalWidth += spacing;
    if (!m_unit.isEmpty()) totalWidth += spacing;

    // 计算起始位置 - 在仪表盘底部绘制信息栏
    int y = height() - 10; // 距离底部10像素
    int x = (width() - totalWidth) / 2;

    // 绘制背景矩形（可选）
    // painter->fillRect(QRect(0, height() - 30, width(), 30), QColor(240, 240, 240));

    // 绘制标签
    if (!m_label.isEmpty()) {
        // 设置标签字体（加粗）
        QFont boldFont = labelFont;
        boldFont.setBold(true);
        painter->setFont(boldFont);

        painter->drawText(x, y, m_label);
        x += labelWidth + spacing;

        // 恢复普通字体
        painter->setFont(labelFont);
    }

    // 绘制数值 - 使用稍大的字体
    QFont valueFont = labelFont;
    valueFont.setPointSize(11);
    painter->setFont(valueFont);
    painter->drawText(x, y, valueText);
    x += valueWidth;

    // 恢复普通字体
    painter->setFont(labelFont);

    // 绘制单位
    if (!m_unit.isEmpty()) {
        x += spacing;
        painter->drawText(x, y, m_unit);
    }

    painter->restore();
}
