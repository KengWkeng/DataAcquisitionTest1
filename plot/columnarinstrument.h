#ifndef COLUMNARINSTRUMENT_H
#define COLUMNARINSTRUMENT_H

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPixmap>
#include <QFont>

class QLabel; // Forward declaration

class ColumnarInstrument : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minValue READ minValue WRITE setMinValue NOTIFY rangeChanged)
    Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue NOTIFY rangeChanged)
    Q_PROPERTY(QString unit READ unit WRITE setUnit NOTIFY unitChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(int precision READ precision WRITE setPrecision NOTIFY precisionChanged)

public:
    explicit ColumnarInstrument(QWidget *parent = nullptr);

    double value() const;
    double minValue() const;
    double maxValue() const;
    QString unit() const;
    QString label() const;
    int precision() const;

    Q_INVOKABLE void configure(const QString &label, const QString &unit, int precision, double minRange, double maxRange);

public slots:
    void setValue(double value);
    void setMinValue(double minValue);
    void setMaxValue(double maxValue);
    void setRange(double minValue, double maxValue);
    void setUnit(const QString &unit);
    void setLabel(const QString &label);
    void setPrecision(int precision); // For digital display

signals:
    void valueChanged(double value);
    void rangeChanged(double minValue, double maxValue);
    void unitChanged(const QString &unit);
    void labelChanged(const QString &label);
    void precisionChanged(int precision);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawBackground(QPainter *painter);
    void drawBar(QPainter *painter);
    void drawScale(QPainter *painter);
    void drawLabels(QPainter *painter);
    void drawAlarms(QPainter *painter);
    void drawBarAreaOutline(QPainter *painter);
    void updateDigitalDisplay();
    void updateStaticCache();

    double m_value = 0.0;
    double m_minValue = 0.0;
    double m_maxValue = 100.0;
    QString m_unit = ""; // Default to empty string to avoid empty string issues
    QString m_label = ""; // Default to empty string
    int m_precision = 1;

    QLabel *m_valueLabel; // Digital display

    // Drawing parameters (can be adjusted)
    int m_scaleMargin = 10;
    int m_barWidthRatio = 40; // Percentage of width
    int m_scaleTicks = 11; // Number of major ticks (0 to 100 -> 11 ticks)
    int m_labelAreaHeight = 35; // Slightly increase for larger font
    int m_digitalDisplayHeight = 40;
    int m_alarmMarkWidth = 5;
    int m_alarmLabelOffset = 2;

    // Colors
    QColor m_barColor = QColor(0, 100, 200); // Normal color (0-70%)
    QColor m_warningColor = QColor(218, 165, 32);      // Warning color (70-85%)
    QColor m_dangerColor = QColor(178, 34, 34);        // Danger color (>85%)
    QColor m_scaleColor = Qt::black;
    QColor m_labelColor = Qt::black;
    QColor m_backgroundColor = Qt::white;
    QColor m_outlineColor = Qt::darkGray; // Color for the bar area outline

    // Thresholds (as percentages of the range)
    double m_warningThreshold = 0.70;
    double m_dangerThreshold = 0.85;

    // Cache for static elements
    QPixmap m_staticCache;
    bool m_cacheDirty = true;

    // Font
    QFont m_topLabelFont;
    QFont m_valueLabelFont;
};

#endif // COLUMNARINSTRUMENT_H 