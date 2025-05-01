#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
#include <QMap>
#include <QThread>
#include "Config/ConfigManager.h"
#include "Device/DeviceManager.h"
#include "Processing/DataProcessor.h"
#include "plot/qcustomplot.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    // 设备相关槽
    void onRawDataPointReady(QString deviceId, QString hardwareChannel, double rawValue, qint64 timestamp);
    void onDeviceStatusChanged(QString deviceId, Core::StatusCode status, QString message);
    void onErrorOccurred(QString deviceId, QString errorMsg);

    // 处理相关槽
    void onProcessedDataPointReady(QString channelId, Core::ProcessedDataPoint dataPoint);
    void onSyncFrameReady(Core::SynchronizedDataFrame frame);
    void onChannelStatusChanged(QString channelId, Core::StatusCode status, QString message);
    void onProcessingError(QString errorMsg);

private slots:
    // 控制采集的槽
    void onStartStopButtonClicked();
    // 更新图表的槽
    void updatePlot();

private:
    void initializeConfig();
    void initializeDevices();
    void initializeProcessing();
    void initializeUI();
    void setupPlot();
    void testConfigManager();
    void testDeviceManager();
    void testDataSynchronizer();

    // 添加通道到图表
    void addChannelToPlot(const QString& channelId, const QString& channelName, const QColor& color);

private:
    Ui::MainWindow *ui;
    Config::ConfigManager *m_configManager;
    Device::DeviceManager *m_deviceManager;
    Processing::DataProcessor *m_dataProcessor;
    QThread *m_processorThread;

    // UI 组件
    QPushButton *m_startStopButton;
    QCustomPlot *m_plot;

    // 数据和状态
    QMap<QString, QCPGraph*> m_channelGraphs;  // 通道ID -> 图表对象
    QMap<QString, QVector<double>> m_timeData; // 通道ID -> 时间数据
    QMap<QString, QVector<double>> m_valueData; // 通道ID -> 值数据
    bool m_isAcquiring;                        // 是否正在采集
    QTimer *m_plotUpdateTimer;                 // 图表更新定时器
    qint64 m_startTimestamp;                   // 采集开始时间戳
};
#endif // MAINWINDOW_H
