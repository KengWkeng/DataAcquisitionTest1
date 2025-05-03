#include "columnarinstrument.h"
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <QResizeEvent>
#include <QApplication> // For default font

ColumnarInstrument::ColumnarInstrument(QWidget *parent)
    : QWidget(parent)
    , m_valueLabel(new QLabel(this))
{
    // Initial Font Setup
    m_topLabelFont = QApplication::font();
    m_topLabelFont.setPointSize(m_topLabelFont.pointSize() + 2); // Increase size slightly
    m_topLabelFont.setBold(true);

    m_valueLabelFont = QApplication::font();
    m_valueLabelFont.setPointSize(m_valueLabelFont.pointSize() + 4); // 比顶部标签更大

    // Layout and QLabel Initialization
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // Add space at the top for the potentially larger label
    mainLayout->setContentsMargins(5, m_labelAreaHeight, 5, 5); // Add some side margins too
    mainLayout->addStretch(); // Push Label to bottom

    m_valueLabel->setFont(m_valueLabelFont);
    m_valueLabel->setAlignment(Qt::AlignCenter); // 居中对齐
    m_valueLabel->setFixedHeight(m_digitalDisplayHeight);
    m_valueLabel->setAutoFillBackground(false); // 通常 QLabel 不需要背景填充
    QPalette valPalette = m_valueLabel->palette();
    valPalette.setColor(QPalette::WindowText, m_labelColor);
    m_valueLabel->setPalette(valPalette);

    mainLayout->addWidget(m_valueLabel);
    setLayout(mainLayout);

    setMinimumSize(120, 250); // Slightly wider minimum
    updateDigitalDisplay();

    // Initial configuration (optional default values)
    // configure("Parameter", "Units", 1, 0.0, 100.0);
}

double ColumnarInstrument::value() const { return m_value; }
double ColumnarInstrument::minValue() const { return m_minValue; }
double ColumnarInstrument::maxValue() const { return m_maxValue; }
QString ColumnarInstrument::unit() const { return m_unit; }
QString ColumnarInstrument::label() const { return m_label; }
int ColumnarInstrument::precision() const { return m_precision; }

void ColumnarInstrument::configure(const QString &label, const QString &unit, int precision, double minRange, double maxRange)
{
    bool changed = false;
    if (m_label != label) { m_label = label; changed = true; emit labelChanged(m_label); }
    if (m_unit != unit) { m_unit = unit; changed = true; emit unitChanged(m_unit); }
    if (m_precision != precision && precision >= 0) { m_precision = precision; changed = true; emit precisionChanged(m_precision); }
    if (minRange < maxRange) {
        if (m_minValue != minRange) { m_minValue = minRange; changed = true; }
        if (m_maxValue != maxRange) { m_maxValue = maxRange; changed = true; }
        if (m_minValue != minRange || m_maxValue != maxRange) {
             emit rangeChanged(m_minValue, m_maxValue);
        }
    }

    if (changed) {
        m_cacheDirty = true;
        setValue(m_value); // Re-clamp value if range changed
        updateDigitalDisplay(); // Update numeric part of display
        update(); // Trigger repaint
    }
}

void ColumnarInstrument::setValue(double value)
{
    double clampedValue = std::clamp(value, m_minValue, m_maxValue);
    if (m_value != clampedValue) {
        m_value = clampedValue;
        updateDigitalDisplay();
        // No need for m_cacheDirty = true here, only bar redraw needed
        update();
        emit valueChanged(m_value);
    }
}

void ColumnarInstrument::setMinValue(double minValue)
{
    configure(m_label, m_unit, m_precision, minValue, m_maxValue);
}

void ColumnarInstrument::setMaxValue(double maxValue)
{
    configure(m_label, m_unit, m_precision, m_minValue, maxValue);
}

void ColumnarInstrument::setRange(double minValue, double maxValue)
{
     configure(m_label, m_unit, m_precision, minValue, maxValue);
}

void ColumnarInstrument::setUnit(const QString &unit)
{
    configure(m_label, unit, m_precision, m_minValue, m_maxValue);
}

void ColumnarInstrument::setLabel(const QString &label)
{
    configure(label, m_unit, m_precision, m_minValue, m_maxValue);
}

void ColumnarInstrument::setPrecision(int precision)
{
    configure(m_label, m_unit, precision, m_minValue, m_maxValue);
}

void ColumnarInstrument::resizeEvent(QResizeEvent *event)
{
    m_cacheDirty = true;
    QWidget::resizeEvent(event);
    // Force immediate layout adjustment for LCD positioning if needed
    // layout()->invalidate();
    // layout()->activate();
}

void ColumnarInstrument::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    if (m_cacheDirty || m_staticCache.isNull() || m_staticCache.size() != size() * devicePixelRatioF()) {
        updateStaticCache();
    }

    // Draw cache (adjusting for high DPI)
    painter.drawPixmap(rect(), m_staticCache);

    // Draw dynamic bar
    painter.setRenderHint(QPainter::Antialiasing);
    drawBar(&painter);
}

void ColumnarInstrument::updateStaticCache()
{
    // Create pixmap with device pixel ratio for high DPI screens
    m_staticCache = QPixmap(size() * devicePixelRatioF());
    m_staticCache.setDevicePixelRatio(devicePixelRatioF());
    m_staticCache.fill(Qt::transparent);

    QPainter cachePainter(&m_staticCache);
    cachePainter.setRenderHint(QPainter::Antialiasing);

    drawBackground(&cachePainter);
    drawScale(&cachePainter);
    drawBarAreaOutline(&cachePainter);
    drawAlarms(&cachePainter);
    drawLabels(&cachePainter); // Only draws top label now

    m_cacheDirty = false;
}

void ColumnarInstrument::drawBackground(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_backgroundColor);
    painter->drawRect(rect());
    painter->restore();
}

void ColumnarInstrument::drawScale(QPainter *painter)
{
    painter->save();
    painter->setPen(m_scaleColor);

    int topMargin = m_labelAreaHeight + m_scaleMargin;
    int bottomMargin = m_digitalDisplayHeight + m_scaleMargin + 5;
    int availableHeight = height() - topMargin - bottomMargin;
    if (availableHeight <= 0) {
         painter->restore();
         return;
    }

    int barAreaWidth = width() * m_barWidthRatio / 100;
    int scaleAreaX = (width() - barAreaWidth) / 2;
    int scaleLineX = scaleAreaX + barAreaWidth;

    double range = m_maxValue - m_minValue;
    if (range <= 0) range = 1;

    QFontMetrics fm = painter->fontMetrics();
    for (int i = 0; i < m_scaleTicks; ++i) {
        double value = m_minValue + (range * i) / (m_scaleTicks - 1);
        double ratio = static_cast<double>(i) / (m_scaleTicks - 1);
        int y = topMargin + availableHeight * (1.0 - ratio);

        painter->drawLine(scaleLineX, y, scaleLineX + 5, y);

        // Use precision for scale labels if appropriate, maybe 0 is fine
        QString tickLabel = QString::number(value, 'f', 0);
        int labelWidth = fm.horizontalAdvance(tickLabel);
        painter->drawText(scaleLineX + 8, y - fm.height() / 2 + fm.ascent(), tickLabel);

        // Minor ticks (corrected calculation)
        if (i < m_scaleTicks - 1) {
            int minorTicks = 4;
            double majorStepRatio = 1.0 / (m_scaleTicks - 1);
            for (int j = 1; j <= minorTicks; ++j) {
                double minorOffsetRatio = (static_cast<double>(j) / (minorTicks + 1)) * majorStepRatio;
                double minorRatio = ratio + minorOffsetRatio;
                int minorY = topMargin + availableHeight * (1.0 - minorRatio);
                if (minorY < height() - bottomMargin && minorY > topMargin) {
                    painter->drawLine(scaleLineX, minorY, scaleLineX + 3, minorY);
                }
            }
        }
    }

    painter->restore();
}

void ColumnarInstrument::drawBar(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);

    int topMargin = m_labelAreaHeight + m_scaleMargin;
    int bottomMargin = m_digitalDisplayHeight + m_scaleMargin + 5;
    int availableHeight = height() - topMargin - bottomMargin;
    if (availableHeight <= 0) {
        painter->restore();
        return;
    }

    int barWidth = width() * m_barWidthRatio / 100;
    int barX = (width() - barWidth) / 2;

    double range = m_maxValue - m_minValue;
    if (range <= 0) range = 1;

    double valueRatio = (m_value - m_minValue) / range;
    valueRatio = std::clamp(valueRatio, 0.0, 1.0);

    int currentBarHeight = static_cast<int>(availableHeight * valueRatio);
    int currentBarY = topMargin + availableHeight - currentBarHeight;

    int warningLineY = topMargin + static_cast<int>(availableHeight * (1.0 - m_warningThreshold));
    int dangerLineY = topMargin + static_cast<int>(availableHeight * (1.0 - m_dangerThreshold));
    int bottomY = topMargin + availableHeight;

    // Draw segments from bottom up
    if (currentBarY < bottomY) {
        int normalTopY = std::max(currentBarY, warningLineY);
        int normalHeight = bottomY - normalTopY;
        if (normalHeight > 0) {
            painter->setBrush(m_barColor);
            painter->drawRect(barX, normalTopY, barWidth, normalHeight);
        }
    }

    if (valueRatio >= m_warningThreshold && currentBarY < warningLineY) {
        int warningTopY = std::max(currentBarY, dangerLineY);
        int warningHeight = warningLineY - warningTopY;
        if (warningHeight > 0) {
            painter->setBrush(m_warningColor);
            painter->drawRect(barX, warningTopY, barWidth, warningHeight);
        }
    }

    if (valueRatio >= m_dangerThreshold && currentBarY < dangerLineY) {
        int dangerHeight = dangerLineY - currentBarY;
        if (dangerHeight > 0) {
            painter->setBrush(m_dangerColor);
            painter->drawRect(barX, currentBarY, barWidth, dangerHeight);
        }
    }

    painter->restore();
}

void ColumnarInstrument::drawBarAreaOutline(QPainter *painter)
{
    painter->save();
    painter->setPen(QPen(m_outlineColor, 1));
    painter->setBrush(Qt::NoBrush);

    int topMargin = m_labelAreaHeight + m_scaleMargin;
    int bottomMargin = m_digitalDisplayHeight + m_scaleMargin + 5;
    int availableHeight = height() - topMargin - bottomMargin;
    if (availableHeight <= 0) {
        painter->restore();
        return;
    }

    int barWidth = width() * m_barWidthRatio / 100;
    int barX = (width() - barWidth) / 2;

    QRectF barAreaRect(barX, topMargin, barWidth, availableHeight);
    painter->drawRect(barAreaRect);

    painter->restore();
}

void ColumnarInstrument::drawAlarms(QPainter *painter)
{
    painter->save();

    int topMargin = m_labelAreaHeight + m_scaleMargin;
    int bottomMargin = m_digitalDisplayHeight + m_scaleMargin + 5;
    int availableHeight = height() - topMargin - bottomMargin;
    if (availableHeight <= 0) {
        painter->restore();
        return;
    }

    double range = m_maxValue - m_minValue;
    if (range <= 0) range = 1;

    int barAreaWidth = width() * m_barWidthRatio / 100;
    int barX = (width() - barAreaWidth) / 2;
    int alarmLineX = barX - m_alarmMarkWidth - m_alarmLabelOffset;

    auto drawAlarmMark = [&](double thresholdRatio, const QString& label, const QColor& markerColor) {
        double alarmValue = m_minValue + range * thresholdRatio;
        if (alarmValue >= m_minValue && alarmValue <= m_maxValue) {
            double ratio = (alarmValue - m_minValue) / range;
            int y = topMargin + static_cast<int>(availableHeight * (1.0 - ratio));

            // Draw Marker Line (Yellow or Red)
            painter->setPen(markerColor);
            painter->setBrush(markerColor);
            painter->drawLine(alarmLineX, y, alarmLineX + m_alarmMarkWidth, y);

            // --- Draw Text (Always Black) ---
            painter->setPen(m_labelColor); // <-- 设置画笔为黑色
            QFontMetrics fm = painter->fontMetrics();
            int labelWidth = fm.horizontalAdvance(label);
            painter->drawText(alarmLineX - labelWidth - m_alarmLabelOffset, y - fm.height() / 2 + fm.ascent(), label);
        }
    };

    drawAlarmMark(m_warningThreshold, "AH", m_warningColor);
    drawAlarmMark(m_dangerThreshold, "AHH", m_dangerColor);

    painter->restore();
}

void ColumnarInstrument::drawLabels(QPainter *painter)
{
    painter->save();
    painter->setPen(m_labelColor);

    // --- Draw Top Label (Title/Unit) ---
    if (!m_label.isEmpty()) {
        painter->setFont(m_topLabelFont);
        QFontMetrics fm = painter->fontMetrics();
        QString fullLabel = m_label;
        if (!m_unit.isEmpty()) {
            fullLabel += ("/" + m_unit);
        }
        int labelWidth = fm.horizontalAdvance(fullLabel);
        int labelX = (width() - labelWidth) / 2;
        // Center vertically within the allocated top space
        int labelY = fm.ascent() + (m_labelAreaHeight - fm.height()) / 2;
        painter->drawText(labelX, labelY, fullLabel);
    }

    painter->restore();
}

void ColumnarInstrument::updateDigitalDisplay()
{
    if (m_valueLabel) {
        QString valueStr = QString::number(m_value, 'f', m_precision);
        QString displayText = valueStr;
        if (!m_unit.isEmpty()) {
            displayText += " " + m_unit;
        }
        m_valueLabel->setText(displayText);
    }
} 