#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Config/ConfigManager.h"
#include "Device/DeviceManager.h"
#include "Processing/DataSynchronizer.h"

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

private:
    void initializeConfig();
    void initializeDevices();
    void initializeProcessing();
    void testConfigManager();
    void testDeviceManager();
    void testDataSynchronizer();

private:
    Ui::MainWindow *ui;
    Config::ConfigManager *m_configManager;
    Device::DeviceManager *m_deviceManager;
    Processing::DataSynchronizer *m_dataSynchronizer;
};
#endif // MAINWINDOW_H
