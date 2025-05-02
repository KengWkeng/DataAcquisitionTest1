#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QSpinBox>
#include <QGridLayout>
#include <QByteArray>
#include <QMap>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 帧数据结构体
struct FrameData {
    float engineSpeed;        // 发动机转速 (rpm)
    float throttlePosition;   // 节气门开度 (%)
    float cylinderTemp;         // 缸温 (°C)
    float exhaustTemp;         // 排温 (°C)
    float fuelPressure;       // 燃油压力 (kPa)
    float rotorTemp;            // 转子温度 (°C)
    float intakeTemp;           // 进气温度 (°C)
    float intakePressure;      // 进气压力
    float supplyVoltage;       // 供电电压 (V)
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updatePortList();
    void connectSerialPort();
    void readData();
    void handleSerialError(QSerialPort::SerialPortError error);
    void clearDisplay();

private:
    Ui::MainWindow *ui;
    QSerialPort *serialPort;
    QComboBox *portComboBox;
    QPushButton *connectButton;
    QPushButton *clearButton;
    QTextEdit *logTextEdit;
    QGridLayout *dataGridLayout;
    QMap<QString, QLabel*> dataLabels;
    QByteArray receiveBuffer;
    
    void setupUi();
    bool parseFrame(const QByteArray &frame, FrameData &data);
    void displayFrameData(const FrameData &data);
    void displayRawHexData(const QByteArray &data);
    void appendLog(const QString &text);
    void processReceivedData(const QByteArray &data);
    bool validateChecksum(const QByteArray &frame);
};
#endif // MAINWINDOW_H
