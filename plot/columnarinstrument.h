#ifndef ColumnarInstrument_H
#define ColumnarInstrument_H

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPixmap>
#include <QFont>
#include <QLabel>
#include <QVBoxLayout>

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
    void setPrecision(int precision);

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
    void drawScale(QPainter *painter);
    void drawScaleLabels(QPainter *painter);
    void drawStaticColorZones(QPainter *painter);
    void drawIndicator(QPainter *painter);
    void updateTextLabels();
    void updateStaticCache();

    double m_value = 0.0;
    double m_minValue = 0.0;
    double m_maxValue = 100.0;
    QString m_unit = "";
    QString m_label = "";
    int m_precision = 1;

    QLabel *m_mainLabel;
    QLabel *m_valueLabel;

    int m_scaleMargin = 5;
    int m_barWidthRatio = 20;
    int m_scaleTicks = 11;
    int m_textWidthRatio = 40;
    int m_indicatorHeight = 3;
    int m_indicatorOverhang = 4;

    QColor m_warningColor = QColor(218, 165, 32);
    QColor m_dangerColor = QColor(178, 34, 34);
    QColor m_normalColor = Qt::white;
    QColor m_scaleColor = Qt::black;
    QColor m_labelColor = Qt::black;
    QColor m_backgroundColor = Qt::white;
    QColor m_indicatorColor = Qt::blue;
    QColor m_outerBorderColor = QColor(200, 200, 200, 150);

    const double m_warningThreshold = 0.70;
    const double m_dangerThreshold = 0.85;

    QPixmap m_staticCache;
    bool m_cacheDirty = true;

    QFont m_topLabelFont;
    QFont m_valueLabelFont;

};

#endif // ColumnarInstrument_H 