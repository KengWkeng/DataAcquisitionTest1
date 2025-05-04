#include "columnarinstrument.h"
#include <QPainter>
#include <QHBoxLayout> // Use QHBoxLayout
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget> // For the text area container
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <QResizeEvent>
#include <QApplication>

// --- 修改类名和包含文件 --- 
ColumnarInstrument::ColumnarInstrument(QWidget *parent)
    : QWidget(parent)
    , m_mainLabel(new QLabel(this))
    , m_valueLabel(new QLabel(this))
{
    // Font Setup (reuse from previous version)
    m_topLabelFont = QApplication::font();
    m_topLabelFont.setPointSize(m_topLabelFont.pointSize() + 1);
    // m_topLabelFont.setBold(true);

    m_valueLabelFont = QApplication::font();
    m_valueLabelFont.setPointSize(m_valueLabelFont.pointSize() + 2);
    m_valueLabelFont.setBold(true);

    // --- Layout Setup (QHBoxLayout) ---
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2); // Minimal margins
    mainLayout->setSpacing(5);

    // Left side: Drawing area (handled by paintEvent on 'this')
    // We need a placeholder or rely on stretch factors
    // Let the text area define its size, drawing area takes the rest

    // Right side: Text Area
    QWidget *textWidget = new QWidget(this);
    QVBoxLayout *textLayout = new QVBoxLayout(textWidget);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    textLayout->addStretch(1);

    m_mainLabel->setFont(m_topLabelFont);
    m_mainLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_mainLabel->setForegroundRole(QPalette::WindowText);
    textLayout->addWidget(m_mainLabel);

    m_valueLabel->setFont(m_valueLabelFont);
    m_valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_valueLabel->setForegroundRole(QPalette::WindowText);
    textLayout->addWidget(m_valueLabel);
    textLayout->addStretch(1);

    textWidget->setLayout(textLayout);

    // Add drawing area stretch and text widget to main layout
    mainLayout->addStretch(100 - m_textWidthRatio); // Drawing area takes majority width
    mainLayout->addWidget(textWidget, m_textWidthRatio); // Text area takes defined ratio

    setLayout(mainLayout);

    setMinimumSize(150, 100); // Adjust minimum size
    updateTextLabels(); // Initialize text labels
}

// Destructor remains simple (Qt handles children)

// --- Getters (unchanged) ---
double ColumnarInstrument::value() const { return m_value; }
double ColumnarInstrument::minValue() const { return m_minValue; }
double ColumnarInstrument::maxValue() const { return m_maxValue; }
QString ColumnarInstrument::unit() const { return m_unit; }
QString ColumnarInstrument::label() const { return m_label; }
int ColumnarInstrument::precision() const { return m_precision; }

// --- Configure Method ---
void ColumnarInstrument::configure(const QString &label, const QString &unit, int precision, double minRange, double maxRange)
{
    bool labelOrUnitChanged = false;
    bool rangeOrPrecisionChanged = false;

    if (m_label != label) { m_label = label; labelOrUnitChanged = true; emit labelChanged(m_label); }
    if (m_unit != unit) { m_unit = unit; labelOrUnitChanged = true; emit unitChanged(m_unit); }
    if (m_precision != precision && precision >= 0) { m_precision = precision; rangeOrPrecisionChanged = true; emit precisionChanged(m_precision); }

    if (minRange < maxRange) {
        bool rangeChangedFlag = false;
        if (m_minValue != minRange) { m_minValue = minRange; rangeChangedFlag = true; }
        if (m_maxValue != maxRange) { m_maxValue = maxRange; rangeChangedFlag = true; }
        if (rangeChangedFlag) {
             rangeOrPrecisionChanged = true;
             emit rangeChanged(m_minValue, m_maxValue);
        }
    }

    if (labelOrUnitChanged || rangeOrPrecisionChanged) {
        m_cacheDirty = true;
        updateTextLabels();
        setValue(m_value); // Re-clamp value if range changed
        update(); // Trigger repaint
    }
}

// --- Setters (call configure) ---
void ColumnarInstrument::setValue(double value)
{
    double clampedValue = std::clamp(value, m_minValue, m_maxValue);
    // Use m_currentValue for smooth indicator movement if needed later,
    // For now, just update m_value directly and redraw indicator.
    // Let's skip animation for simplicity first.
    if (m_value != clampedValue) {
        m_value = clampedValue;
        // update m_currentValue if animation was used
        updateTextLabels(); // Update the value display label
        update(); // Trigger repaint for the indicator
        emit valueChanged(m_value);
    }
}

void ColumnarInstrument::setMinValue(double minValue) { configure(m_label, m_unit, m_precision, minValue, m_maxValue); }
void ColumnarInstrument::setMaxValue(double maxValue) { configure(m_label, m_unit, m_precision, m_minValue, maxValue); }
void ColumnarInstrument::setRange(double minValue, double maxValue) { configure(m_label, m_unit, m_precision, minValue, maxValue); }
void ColumnarInstrument::setUnit(const QString &unit) { configure(m_label, unit, m_precision, m_minValue, m_maxValue); }
void ColumnarInstrument::setLabel(const QString &label) { configure(label, m_unit, m_precision, m_minValue, m_maxValue); }
void ColumnarInstrument::setPrecision(int precision) { configure(m_label, m_unit, precision, m_minValue, m_maxValue); }


// --- Update Text Labels ---
void ColumnarInstrument::updateTextLabels()
{
    if (m_mainLabel) {
        QString mainText = m_label;
        if (!m_unit.isEmpty()) {
            mainText += "/" + m_unit;
        }
        m_mainLabel->setText(mainText);
    }
    if (m_valueLabel) {
        QString valueStr = QString::number(m_value, 'f', m_precision);
        QString valueText = valueStr;
        if (!m_unit.isEmpty()) {
            valueText += " " + m_unit;
        }
        m_valueLabel->setText(valueText);
    }
}

// --- Event Handlers ---
void ColumnarInstrument::resizeEvent(QResizeEvent *event)
{
    m_cacheDirty = true;
    QWidget::resizeEvent(event);
    update(); // Trigger repaint
}

void ColumnarInstrument::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Calculate drawing area (left part of the widget)
    int drawingWidth = width() * (100 - m_textWidthRatio) / 100 - layout()->spacing();
    QRect drawingRect(0, 0, drawingWidth, height());

    // Update static cache if needed
    // Use drawingRect size for cache? No, cache the whole widget bg maybe?
    // Let's cache only the drawing area part.
    QPixmap currentCache = m_staticCache;
    if (m_cacheDirty || currentCache.isNull() || currentCache.size() != drawingRect.size() * devicePixelRatioF()) {
        updateStaticCache();
        currentCache = m_staticCache; // Get updated cache
    }

    // Draw static cache for the drawing area
    if (!currentCache.isNull()) {
        painter.drawPixmap(drawingRect.topLeft(), currentCache);
    }

    // Draw dynamic indicator within the drawing area
    painter.save();
    painter.translate(drawingRect.topLeft()); // Adjust painter for drawing area
    drawIndicator(&painter);
    painter.restore();

    // Draw outer border for the whole widget
    painter.setPen(QPen(m_outerBorderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

// --- Static Cache Update ---
void ColumnarInstrument::updateStaticCache()
{
    // --- 内部计算绘图区域大小 ---
    int drawingWidth = width() * (100 - m_textWidthRatio) / 100 - layout()->spacing();
    QSize cacheSize(drawingWidth, height());
    if (cacheSize.width() <= 0 || cacheSize.height() <= 0) return; // Avoid creating empty pixmap

    m_staticCache = QPixmap(cacheSize * devicePixelRatioF());
    m_staticCache.setDevicePixelRatio(devicePixelRatioF());
    m_staticCache.fill(m_backgroundColor);

    QPainter cachePainter(&m_staticCache);
    cachePainter.setRenderHint(QPainter::Antialiasing);

    // Draw static elements relative to the cache pixmap (size = cacheSize)
    drawStaticColorZones(&cachePainter);
    drawScale(&cachePainter);
    drawScaleLabels(&cachePainter);

    m_cacheDirty = false;
}

// --- Drawing Methods ---

void ColumnarInstrument::drawBackground(QPainter *painter)
{
    // Background is now drawn when filling the cache
    // Or could draw widget background here if cache was transparent
    Q_UNUSED(painter);
}

void ColumnarInstrument::drawStaticColorZones(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);

    // Calculate drawing geometry within the provided areaSize
    int drawingWidth = width() * (100 - m_textWidthRatio) / 100 - layout()->spacing();
    QSize areaSize(drawingWidth, height());

    int availableHeight = areaSize.height() - 2 * m_scaleMargin;
    if (availableHeight <= 0) {
        painter->restore();
        return;
    }
    int barWidth = areaSize.width() * m_barWidthRatio / 100;
    // Position bar slightly off the left edge, leave space for scale
    int scaleAreaWidth = 30; // Width reserved for scale ticks/labels
    int barX = scaleAreaWidth + m_scaleMargin;
    int topY = m_scaleMargin;
    int bottomY = topY + availableHeight;

    // Calculate Y coordinates for thresholds
    int warningLineY = topY + static_cast<int>(availableHeight * (1.0 - m_warningThreshold));
    int dangerLineY = topY + static_cast<int>(availableHeight * (1.0 - m_dangerThreshold));

    // 1. Danger Zone (Red)
    painter->setBrush(m_dangerColor);
    painter->drawRect(barX, topY, barWidth, dangerLineY - topY);

    // 2. Warning Zone (Yellow)
    painter->setBrush(m_warningColor);
    painter->drawRect(barX, dangerLineY, barWidth, warningLineY - dangerLineY);

    // 3. Normal Zone (White)
    painter->setBrush(m_normalColor);
    painter->drawRect(barX, warningLineY, barWidth, bottomY - warningLineY);

    // Optional: Outline around the zones
    painter->setPen(m_scaleColor);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(barX, topY, barWidth, availableHeight);

    painter->restore();
}

void ColumnarInstrument::drawScale(QPainter *painter)
{
    painter->save();
    painter->setPen(m_scaleColor);

    int drawingWidth = width() * (100 - m_textWidthRatio) / 100 - layout()->spacing();
    QSize areaSize(drawingWidth, height());

    int availableHeight = areaSize.height() - 2 * m_scaleMargin;
    if (availableHeight <= 0) {
        painter->restore();
        return;
    }
    // Position scale to the left of the bar
    int scaleAreaWidth = 30;
    int scaleLineX = scaleAreaWidth; // Right edge of scale area
    int topY = m_scaleMargin;

    double range = m_maxValue - m_minValue;
    if (range <= 0) range = 1;

    for (int i = 0; i < m_scaleTicks; ++i) {
        double ratio = static_cast<double>(i) / (m_scaleTicks - 1);
        int y = topY + availableHeight * (1.0 - ratio);

        // Major tick mark (longer, points right towards bar)
        painter->drawLine(scaleLineX - 5, y, scaleLineX, y);

        // Optional: Minor ticks
        if (i < m_scaleTicks - 1) {
            int minorTicks = 4;
            double majorStepRatio = 1.0 / (m_scaleTicks - 1);
            for (int j = 1; j <= minorTicks; ++j) {
                double minorOffsetRatio = (static_cast<double>(j) / (minorTicks + 1)) * majorStepRatio;
                double minorRatio = ratio + minorOffsetRatio;
                int minorY = topY + availableHeight * (1.0 - minorRatio);
                if (minorY < areaSize.height() - m_scaleMargin && minorY > m_scaleMargin) {
                    // Shorter minor tick
                    painter->drawLine(scaleLineX - 3, minorY, scaleLineX, minorY);
                }
            }
        }
    }

    painter->restore();
}

void ColumnarInstrument::drawScaleLabels(QPainter *painter)
{
    painter->save();
    painter->setPen(m_scaleColor);
    painter->setFont(QApplication::font()); // Use default font

    int drawingWidth = width() * (100 - m_textWidthRatio) / 100 - layout()->spacing();
    QSize areaSize(drawingWidth, height());

    int availableHeight = areaSize.height() - 2 * m_scaleMargin;
    if (availableHeight <= 0) {
        painter->restore();
        return;
    }
    int scaleAreaWidth = 30;
    int labelX = 2; // X position for labels (left aligned)
    int topY = m_scaleMargin;

    double range = m_maxValue - m_minValue;
    if (range <= 0) range = 1;

    QFontMetrics fm = painter->fontMetrics();
    for (int i = 0; i < m_scaleTicks; ++i) {
        double value = m_minValue + (range * i) / (m_scaleTicks - 1);
        double ratio = static_cast<double>(i) / (m_scaleTicks - 1);
        int y = topY + availableHeight * (1.0 - ratio);

        QString tickLabel = QString::number(std::round(value)); // Integer labels
        painter->drawText(labelX, y + fm.ascent() / 2, tickLabel);
    }

    painter->restore();
}

void ColumnarInstrument::drawIndicator(QPainter *painter)
{
    painter->save();
    painter->setPen(QPen(m_indicatorColor, m_indicatorHeight)); // Pen width defines line height
    painter->setBrush(m_indicatorColor);

    int drawingWidth = width() * (100 - m_textWidthRatio) / 100 - layout()->spacing();
    QSize areaSize(drawingWidth, height());

    int availableHeight = areaSize.height() - 2 * m_scaleMargin;
    if (availableHeight <= 0) {
        painter->restore();
        return;
    }
    int scaleAreaWidth = 30;
    int barWidth = areaSize.width() * m_barWidthRatio / 100;
    int barX = scaleAreaWidth + m_scaleMargin;
    int topY = m_scaleMargin;

    double range = m_maxValue - m_minValue;
    if (range <= 0) range = 1;

    // Calculate Y position based on m_value (not animated m_currentValue for now)
    double valueRatio = (m_value - m_minValue) / range;
    valueRatio = std::clamp(valueRatio, 0.0, 1.0);
    int indicatorY = topY + static_cast<int>(availableHeight * (1.0 - valueRatio));

    // Calculate X start/end for the indicator line
    int startX = barX - m_indicatorOverhang;
    int endX = barX + barWidth + m_indicatorOverhang;

    painter->drawLine(startX, indicatorY, endX, indicatorY);

    painter->restore();
} 
