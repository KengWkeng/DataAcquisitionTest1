#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serialPort(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("ECU串口接收器");
    
    // 初始化组件
    serialPort = new QSerialPort(this);
    
    // 设置界面
    setupUi();
    
    // 连接信号槽
    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(serialPort, &QSerialPort::errorOccurred, this, &MainWindow::handleSerialError);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectSerialPort);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearDisplay);
    
    // 初始化串口列表
    updatePortList();
    
    // 设置窗口大小
    resize(800, 600);
}

MainWindow::~MainWindow()
{
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }
    delete ui;
}

void MainWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // 串口设置区域
    QGroupBox *serialGroupBox = new QGroupBox("串口设置", this);
    QHBoxLayout *serialLayout = new QHBoxLayout(serialGroupBox);
    
    QLabel *portLabel = new QLabel("端口：", this);
    portComboBox = new QComboBox(this);
    QPushButton *refreshButton = new QPushButton("刷新", this);
    connectButton = new QPushButton("连接", this);
    
    serialLayout->addWidget(portLabel);
    serialLayout->addWidget(portComboBox);
    serialLayout->addWidget(refreshButton);
    serialLayout->addWidget(connectButton);
    
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::updatePortList);
    
    mainLayout->addWidget(serialGroupBox);
    
    // 数据显示区域
    QGroupBox *dataGroupBox = new QGroupBox("解析数据", this);
    dataGridLayout = new QGridLayout(dataGroupBox);
    
    // 创建标签
    QStringList labels = {
        "发动机转速 (rpm):",
        "节气门开度 (%):",
        "缸温 (°C):",
        "排温 (°C):",
        "燃油压力 (kPa):",
        "转子温度 (°C):",
        "进气温度 (°C):",
        "进气压力:",
        "供电电压 (V):"
    };
    
    QStringList dataKeys = {
        "engineSpeed",
        "throttlePosition",
        "cylinderTemp",
        "exhaustTemp",
        "fuelPressure",
        "rotorTemp",
        "intakeTemp",
        "intakePressure",
        "supplyVoltage"
    };
    
    for (int i = 0; i < labels.size(); ++i) {
        QLabel *nameLabel = new QLabel(labels[i], this);
        QLabel *valueLabel = new QLabel("--", this);
        valueLabel->setMinimumWidth(100);
        
        dataGridLayout->addWidget(nameLabel, i, 0);
        dataGridLayout->addWidget(valueLabel, i, 1);
        
        dataLabels[dataKeys[i]] = valueLabel;
    }
    
    mainLayout->addWidget(dataGroupBox);
    
    // 原始数据日志区域
    QGroupBox *logGroupBox = new QGroupBox("接收日志", this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroupBox);
    
    logTextEdit = new QTextEdit(this);
    logTextEdit->setReadOnly(true);
    clearButton = new QPushButton("清除日志", this);
    
    logLayout->addWidget(logTextEdit);
    logLayout->addWidget(clearButton);
    
    mainLayout->addWidget(logGroupBox);
    
    mainLayout->setStretch(0, 1);  // 串口设置区域
    mainLayout->setStretch(1, 3);  // 数据显示区域
    mainLayout->setStretch(2, 6);  // 日志区域
}

void MainWindow::updatePortList()
{
    QString currentPort = portComboBox->currentText();
    
    portComboBox->clear();
    
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QString portDescription = info.portName();
        if (!info.description().isEmpty()) {
            portDescription += " (" + info.description() + ")";
        }
        portComboBox->addItem(portDescription, info.portName());
    }
    
    // 尝试恢复之前选择的端口
    int index = portComboBox->findText(currentPort, Qt::MatchStartsWith);
    if (index >= 0) {
        portComboBox->setCurrentIndex(index);
    }
}

void MainWindow::connectSerialPort()
{
    if (serialPort->isOpen()) {
        serialPort->close();
        connectButton->setText("连接");
        appendLog("串口已断开连接");
        return;
    }
    
    QString portName = portComboBox->currentData().toString();
    if (portName.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择一个串口");
        return;
    }
    
    serialPort->setPortName(portName);
    serialPort->setBaudRate(QSerialPort::Baud115200);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    
    if (serialPort->open(QIODevice::ReadOnly)) {
        connectButton->setText("断开");
        appendLog("已连接到串口 " + portName + "，波特率：115200");
    } else {
        QMessageBox::critical(this, "错误", "无法打开串口：" + serialPort->errorString());
    }
}

void MainWindow::readData()
{
    QByteArray data = serialPort->readAll();
    if (data.isEmpty()) return;
    
    // 处理接收到的数据
    processReceivedData(data);
}

void MainWindow::processReceivedData(const QByteArray &data)
{
    // 将新数据添加到缓冲区
    receiveBuffer.append(data);
    
    // 显示原始十六进制数据
    displayRawHexData(data);
    
    // 查找完整的帧并解析
    while (receiveBuffer.size() >= 17) {  // 最小帧长度
        // 查找帧头 (0x7F 0x7F)
        int startIndex = receiveBuffer.indexOf(QByteArray("\x7F\x7F", 2));
        
        if (startIndex < 0) {
            // 没有找到帧头，清空缓冲区
            receiveBuffer.clear();
            break;
        }
        
        // 移除帧头之前的数据
        if (startIndex > 0) {
            receiveBuffer.remove(0, startIndex);
        }
        
        // 检查是否有完整的帧
        if (receiveBuffer.size() < 17) {
            // 数据不足，等待更多数据
            break;
        }
        
        // 检查帧尾 (0x0D 0x0A)
        if (static_cast<unsigned char>(receiveBuffer.at(15)) == 0x0D && 
            static_cast<unsigned char>(receiveBuffer.at(16)) == 0x0A) {
            
            // 提取完整帧
            QByteArray frame = receiveBuffer.mid(0, 17);
            
            // 验证校验和
            if (validateChecksum(frame)) {
                FrameData frameData;
                if (parseFrame(frame, frameData)) {
                    displayFrameData(frameData);
                    appendLog("成功解析一帧数据");
                } else {
                    appendLog("帧格式错误，无法解析");
                }
            } else {
                appendLog("校验和错误，帧被丢弃");
            }
            
            // 移除已处理的帧
            receiveBuffer.remove(0, 17);
        } else {
            // 帧尾不匹配，移除第一个字节并继续寻找帧头
            receiveBuffer.remove(0, 1);
        }
    }
}

bool MainWindow::validateChecksum(const QByteArray &frame)
{
    if (frame.size() < 15) return false;
    
    unsigned char calculatedChecksum = 0;
    for (int i = 0; i <= 13; ++i) {
        calculatedChecksum += static_cast<unsigned char>(frame.at(i));
    }
    
    unsigned char receivedChecksum = static_cast<unsigned char>(frame.at(14));
    
    return calculatedChecksum == receivedChecksum;
}

bool MainWindow::parseFrame(const QByteArray &frame, FrameData &data)
{
    if (frame.size() < 17) return false;

    // 解析转速 (字节2-3) - 假设原始值即为RPM，直接转为float
    quint16 rawEngineSpeed = static_cast<quint8>(frame.at(2)) | (static_cast<quint16>(static_cast<quint8>(frame.at(3))) << 8);
    data.engineSpeed = static_cast<float>(rawEngineSpeed);

    // 解析节气门开度 (字节4-5) - 原始值需除以10.0得到百分比
    quint16 rawThrottlePosition = static_cast<quint8>(frame.at(4)) | (static_cast<quint16>(static_cast<quint8>(frame.at(5))) << 8);
    data.throttlePosition = static_cast<float>(rawThrottlePosition) / 10.0f; // 使用 10.0f 进行浮点除法

    // 解析缸温 (字节6) - 原始值减去偏移量后转为float
    qint8 rawCylinderTemp = static_cast<qint8>(frame.at(6));
    data.cylinderTemp = static_cast<float>(rawCylinderTemp - 40.0);

    // 解析排温 (字节7) - 原始值乘以系数后转为float
    quint8 rawExhaustTemp = static_cast<quint8>(frame.at(7));
    data.exhaustTemp = static_cast<float>(rawExhaustTemp * 5.0 - 40.0); // 系数为5.0，偏移量为40.0

    // 解析燃油压力 (字节8-9) - 假设原始值即为kPa，直接转为float
    quint16 rawFuelPressure = static_cast<quint8>(frame.at(8)) | (static_cast<quint16>(static_cast<quint8>(frame.at(9))) << 8);
    data.fuelPressure = static_cast<float>(rawFuelPressure);

    // 解析转子温度 (字节10) - 原始值减去偏移量后转为float
    qint8 rawRotorTemp = static_cast<qint8>(frame.at(10));
    data.rotorTemp = static_cast<float>(rawRotorTemp - 40);

    // 解析进气温度 (字节11) - 原始值减去偏移量后转为float
    qint8 rawIntakeTemp = static_cast<qint8>(frame.at(11));
    data.intakeTemp = static_cast<float>(rawIntakeTemp - 40);

    // 解析进气压力 (字节12) - 除以10，直接转为float
    quint8 rawIntakePressure = static_cast<quint8>(frame.at(12));
    data.intakePressure = static_cast<float>(rawIntakePressure) / 1.0f; // 使用 10.0f 进行浮点除法

    // 解析供电电压 (字节13) - 原始值除以10.0得到电压
    quint8 rawSupplyVoltage = static_cast<quint8>(frame.at(13));
    data.supplyVoltage = static_cast<float>(rawSupplyVoltage) / 10.0f; // 使用 10.0f

    return true;
}

void MainWindow::displayFrameData(const FrameData &data)
{
    // 更新UI标签
    dataLabels["engineSpeed"]->setText(QString::number(data.engineSpeed));
    // 节气门开度直接显示原始值
    dataLabels["throttlePosition"]->setText(QString::number(data.throttlePosition, 'f', 1));
    dataLabels["cylinderTemp"]->setText(QString::number(data.cylinderTemp));
    dataLabels["exhaustTemp"]->setText(QString::number(data.exhaustTemp));
    dataLabels["fuelPressure"]->setText(QString::number(data.fuelPressure));
    dataLabels["rotorTemp"]->setText(QString::number(data.rotorTemp));
    dataLabels["intakeTemp"]->setText(QString::number(data.intakeTemp));
    dataLabels["intakePressure"]->setText(QString::number(data.intakePressure));
    dataLabels["supplyVoltage"]->setText(QString::number(data.supplyVoltage, 'f', 1));
}

void MainWindow::displayRawHexData(const QByteArray &data)
{
    QString hexString;
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    
    for (char byte : data) {
        hexString += QString("%1 ").arg(static_cast<quint8>(byte), 2, 16, QChar('0')).toUpper();
    }
    
    appendLog(QString("[%1] 接收: %2").arg(timestamp).arg(hexString));
}

void MainWindow::appendLog(const QString &text)
{
    logTextEdit->append(text);
    // 滚动到底部
    QScrollBar *scrollBar = logTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::clearDisplay()
{
    logTextEdit->clear();
}

void MainWindow::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }
    
    // 只处理重要错误
    switch (error) {
    case QSerialPort::DeviceNotFoundError:
    case QSerialPort::PermissionError:
    case QSerialPort::OpenError:
    case QSerialPort::NotOpenError:
    case QSerialPort::WriteError:
    case QSerialPort::ReadError:
    case QSerialPort::ResourceError:
    case QSerialPort::UnsupportedOperationError:
        QMessageBox::critical(this, "串口错误", serialPort->errorString());
        if (serialPort->isOpen()) {
            serialPort->close();
            connectButton->setText("连接");
        }
        break;
    default:
        break;
    }
}
