#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Config/ConfigManager.h"
#include "Device/DeviceManager.h"

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
    void onRawDataPointReady(QString deviceId, QString hardwareChannel, double rawValue, qint64 timestamp);
    void onDeviceStatusChanged(QString deviceId, Core::StatusCode status, QString message);
    void onErrorOccurred(QString deviceId, QString errorMsg);

private:
    void initializeConfig();
    void initializeDevices();
    void testConfigManager();
    void testDeviceManager();

private:
    Ui::MainWindow *ui;
    Config::ConfigManager *m_configManager;
    Device::DeviceManager *m_deviceManager;
};
#endif // MAINWINDOW_H
