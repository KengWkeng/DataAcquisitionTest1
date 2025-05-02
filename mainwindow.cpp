#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_configManager(nullptr)
    , m_deviceManager(nullptr)
    , m_dataProcessor(nullptr)
    , m_processorThread(nullptr)
    , m_startStopButton(nullptr)
    , m_plot(nullptr)
    , m_isAcquiring(false)
    , m_plotUpdateTimer(nullptr)
    , m_startTimestamp(0)
    , m_displayPointCount(600)  // 默认显示600个点
    , m_timeWindow(60.0)        // 默认显示60秒的数据
{
    ui->setupUi(this);

    // 初始化配置
    initializeConfig();

    // 初始化设备
    initializeDevices();

    // 初始化处理
    initializeProcessing();

    // 初始化UI
    initializeUI();

    // 测试配置管理器
    testConfigManager();

    // 测试设备管理器
    testDeviceManager();

    // 测试数据同步器
    testDataSynchronizer();
}

MainWindow::~MainWindow()
{
    // 停止图表更新定时器
    if (m_plotUpdateTimer) {
        m_plotUpdateTimer->stop();
        delete m_plotUpdateTimer;
        m_plotUpdateTimer = nullptr;
    }

    // 清理数据处理器和线程
    if (m_dataProcessor) {
        // 如果正在采集，确保数据被保存
        if (m_isAcquiring) {
            // 停止数据存储（确保数据被保存）
            QMetaObject::invokeMethod(m_dataProcessor, &Processing::DataProcessor::stopDataStorage, Qt::BlockingQueuedConnection);
            qDebug() << "程序退出时保存数据";
        }

        // 停止处理
        m_dataProcessor->stopProcessing();
    }

    if (m_processorThread) {
        // 停止并等待线程结束
        m_processorThread->quit();
        m_processorThread->wait();
        delete m_processorThread;
        m_processorThread = nullptr;
        // m_dataProcessor 会在线程结束时自动删除
    }

    // 清理设备管理器
    if (m_deviceManager) {
        // 停止所有设备
        m_deviceManager->stopAllDevices();

        // 断开所有设备
        m_deviceManager->disconnectAllDevices();

        delete m_deviceManager;
        m_deviceManager = nullptr;
    }

    // 清理配置管理器
    if (m_configManager) {
        delete m_configManager;
        m_configManager = nullptr;
    }

    delete ui;
}

void MainWindow::initializeConfig()
{
    // 创建配置管理器
    m_configManager = new Config::ConfigManager(this);

    // 检查源配置文件是否存在
    QString sourceConfigPath = QDir::currentPath() + "/config.json";
    QString buildConfigPath = QCoreApplication::applicationDirPath() + "/config.json";
    QString simplifiedConfigPath = QCoreApplication::applicationDirPath() + "/simplified_config.json";

    // 如果源配置文件存在但构建目录中不存在，则复制
    QFile sourceFile(sourceConfigPath);
    QFile buildFile(buildConfigPath);

    if (sourceFile.exists() && !buildFile.exists()) {
        // qDebug() << "源配置文件存在，尝试复制到构建目录...";
        if (QFile::copy(sourceConfigPath, buildConfigPath)) {
            // qDebug() << "配置文件已复制到构建目录";
        } else {
            // qDebug() << "无法复制配置文件到构建目录";
        }
    }

    // qDebug() << "尝试加载配置文件:" << buildConfigPath;
    bool success = m_configManager->loadConfig(buildConfigPath);

    if (!success) {
        // qDebug() << "配置文件加载失败! 尝试创建简化版配置文件...";

        // 创建简化版配置
        QJsonObject rootObj;
        rootObj["synchronization_interval_ms"] = 100;

        // 添加一个虚拟设备
        QJsonArray virtualDevicesArray;
        QJsonObject deviceObj;
        deviceObj["instance_name"] = "Test_Sine_Wave";
        deviceObj["signal_type"] = "sine";
        deviceObj["amplitude"] = 5.0;
        deviceObj["frequency"] = 10.0;

        // 添加通道参数
        QJsonObject channelParamsObj;
        channelParamsObj["gain"] = 1.0;
        channelParamsObj["offset"] = 0.0;

        // 添加校准参数
        QJsonObject calibrationParamsObj;
        calibrationParamsObj["a"] = 0.0;
        calibrationParamsObj["b"] = 0.0;
        calibrationParamsObj["c"] = 1.0;
        calibrationParamsObj["d"] = 0.0;

        channelParamsObj["calibration_params"] = calibrationParamsObj;
        deviceObj["channel_params"] = channelParamsObj;

        virtualDevicesArray.append(deviceObj);
        rootObj["virtual_devices"] = virtualDevicesArray;

        // 创建JSON文档
        QJsonDocument jsonDoc(rootObj);

        // 保存到文件
        QFile file(simplifiedConfigPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(jsonDoc.toJson(QJsonDocument::Indented));
            file.close();
            // qDebug() << "简化版配置文件已创建:" << simplifiedConfigPath;

            // 尝试加载简化版配置
            success = m_configManager->loadConfig(simplifiedConfigPath);
            if (!success) {
                // qDebug() << "简化版配置文件加载失败!";
            } else {
                // qDebug() << "简化版配置文件加载成功!";
            }
        } else {
            // qDebug() << "无法创建简化版配置文件:" << file.errorString();
        }
    } else {
        // qDebug() << "配置文件加载成功!";

        // 尝试保存当前配置到新文件，以确保格式正确
        QString savedConfigPath = QCoreApplication::applicationDirPath() + "/saved_config.json";
        if (m_configManager->saveConfig(savedConfigPath)) {
            // qDebug() << "当前配置已保存到:" << savedConfigPath;
        }
    }
}

void MainWindow::initializeDevices()
{
    // 创建设备管理器
    m_deviceManager = new Device::DeviceManager(this);

    // 连接设备管理器信号
    connect(m_deviceManager, &Device::DeviceManager::rawDataPointReady,
            this, &MainWindow::onRawDataPointReady);
    connect(m_deviceManager, &Device::DeviceManager::deviceStatusChanged,
            this, &MainWindow::onDeviceStatusChanged);
    connect(m_deviceManager, &Device::DeviceManager::errorOccurred,
            this, &MainWindow::onErrorOccurred);

    // 检查配置管理器是否已初始化
    if (!m_configManager) {
        qDebug() << "配置管理器未初始化，无法创建设备!";
        return;
    }

    // 获取虚拟设备配置
    QList<Core::VirtualDeviceConfig> virtualDevices = m_configManager->getVirtualDeviceConfigs();

    // 创建虚拟设备
    if (!virtualDevices.isEmpty()) {
        bool success = m_deviceManager->createVirtualDevices(virtualDevices);
        if (success) {
            qDebug() << "成功创建" << virtualDevices.size() << "个虚拟设备";
        } else {
            qDebug() << "创建虚拟设备失败!";
        }
    } else {
        qDebug() << "没有虚拟设备配置";
    }

    // 获取Modbus设备配置
    QList<Core::ModbusDeviceConfig> modbusDevices = m_configManager->getModbusDeviceConfigs();

    // 创建Modbus设备
    if (!modbusDevices.isEmpty()) {
        bool success = m_deviceManager->createModbusDevices(modbusDevices);
        if (success) {
            qDebug() << "成功创建" << modbusDevices.size() << "个Modbus设备";
        } else {
            qDebug() << "创建Modbus设备失败!";
        }
    } else {
        qDebug() << "没有Modbus设备配置";
    }

    // 获取DAQ设备配置
    QList<Core::DAQDeviceConfig> daqDevices = m_configManager->getDAQDeviceConfigs();

    // 创建DAQ设备
    if (!daqDevices.isEmpty()) {
        bool success = m_deviceManager->createDAQDevices(daqDevices);
        if (success) {
            qDebug() << "成功创建" << daqDevices.size() << "个DAQ设备";
        } else {
            qDebug() << "创建DAQ设备失败!";
        }
    } else {
        qDebug() << "没有DAQ设备配置";
    }

    // 获取ECU设备配置
    QList<Core::ECUDeviceConfig> ecuDevices = m_configManager->getECUDeviceConfigs();

    // 创建ECU设备
    if (!ecuDevices.isEmpty()) {
        bool success = m_deviceManager->createECUDevices(ecuDevices);
        if (success) {
            qDebug() << "成功创建" << ecuDevices.size() << "个ECU设备";
        } else {
            qDebug() << "创建ECU设备失败!";
        }
    } else {
        qDebug() << "没有ECU设备配置";
    }
}

void MainWindow::testConfigManager()
{
    if (!m_configManager) {
        // qDebug() << "配置管理器未初始化!";
        return;
    }

    // 获取同步间隔
    int syncInterval = m_configManager->getSynchronizationIntervalMs();
    // qDebug() << "同步间隔:" << syncInterval << "毫秒";

    // 获取虚拟设备配置
    QList<Core::VirtualDeviceConfig> virtualDevices = m_configManager->getVirtualDeviceConfigs();
    // qDebug() << "虚拟设备数量:" << virtualDevices.size();

    // 打印每个虚拟设备的信息
    for (const auto& device : virtualDevices) {
        // qDebug() << "设备ID:" << device.deviceId;
        // qDebug() << "  实例名称:" << device.instanceName;
        // qDebug() << "  信号类型:" << device.signalType;
        // qDebug() << "  振幅:" << device.amplitude;
        // qDebug() << "  频率:" << device.frequency;
        // qDebug() << "  增益:" << device.channelParams.gain;
        // qDebug() << "  偏移:" << device.channelParams.offset;
        // qDebug() << "  校准参数:";
        // qDebug() << "    a:" << device.channelParams.calibrationParams.a;
        // qDebug() << "    b:" << device.channelParams.calibrationParams.b;
        // qDebug() << "    c:" << device.channelParams.calibrationParams.c;
        // qDebug() << "    d:" << device.channelParams.calibrationParams.d;
    }

    // 获取通道配置
    QMap<QString, Core::ChannelConfig> channelConfigs = m_configManager->getChannelConfigs();
    // qDebug() << "通道数量:" << channelConfigs.size();

    // 打印每个通道的信息
    for (auto it = channelConfigs.constBegin(); it != channelConfigs.constEnd(); ++it) {
        // qDebug() << "通道ID:" << it.key();
        // qDebug() << "  通道名称:" << it.value().channelName;
        // qDebug() << "  设备ID:" << it.value().deviceId;
        // qDebug() << "  硬件通道:" << it.value().hardwareChannel;
        // qDebug() << "  增益:" << it.value().params.gain;
        // qDebug() << "  偏移:" << it.value().params.offset;
    }
}

void MainWindow::testDeviceManager()
{
    if (!m_deviceManager) {
        // qDebug() << "设备管理器未初始化!";
        return;
    }

    // 获取所有设备
    QMap<QString, Device::AbstractDevice*> devices = m_deviceManager->getDevices();
    // qDebug() << "设备数量:" << devices.size();

    // 打印每个设备的信息
    for (auto it = devices.constBegin(); it != devices.constEnd(); ++it) {
        // qDebug() << "设备ID:" << it.key();
        // qDebug() << "  设备类型:" << Core::deviceTypeToString(it.value()->getDeviceType());
        // qDebug() << "  设备状态:" << Core::statusCodeToString(it.value()->getStatus());
    }

    // 启动所有设备
    bool success = m_deviceManager->startAllDevices();
    if (success) {
        // qDebug() << "成功启动所有设备";
    } else {
        // qDebug() << "启动设备失败!";
    }
}

void MainWindow::initializeProcessing()
{
    // 创建数据处理器线程
    m_processorThread = new QThread(this);

    // 创建数据处理器（不设置父对象，以便可以移动到线程）
    int syncIntervalMs = m_configManager ? m_configManager->getSynchronizationIntervalMs() : Core::DEFAULT_SYNC_INTERVAL_MS;
    m_dataProcessor = new Processing::DataProcessor(syncIntervalMs);

    // 将数据处理器移动到线程
    m_dataProcessor->moveToThread(m_processorThread);

    // 连接线程启动和结束信号
    connect(m_processorThread, &QThread::started, [this]() {
        // qDebug() << "数据处理器线程已启动，线程ID:" << QThread::currentThreadId();
    });

    connect(m_processorThread, &QThread::finished, m_dataProcessor, &QObject::deleteLater);

    // 连接数据处理器信号（使用Qt::QueuedConnection确保线程安全）
    connect(m_dataProcessor, &Processing::DataProcessor::processedDataPointReady,
            this, &MainWindow::onProcessedDataPointReady, Qt::QueuedConnection);
    connect(m_dataProcessor, &Processing::DataProcessor::syncFrameReady,
            this, &MainWindow::onSyncFrameReady, Qt::QueuedConnection);
    connect(m_dataProcessor, &Processing::DataProcessor::channelStatusChanged,
            this, &MainWindow::onChannelStatusChanged, Qt::QueuedConnection);
    connect(m_dataProcessor, &Processing::DataProcessor::errorOccurred,
            this, &MainWindow::onProcessingError, Qt::QueuedConnection);
    connect(m_dataProcessor, &Processing::DataProcessor::storageStatusChanged,
            this, &MainWindow::onStorageStatusChanged, Qt::QueuedConnection);
    connect(m_dataProcessor, &Processing::DataProcessor::storageError,
            this, &MainWindow::onStorageError, Qt::QueuedConnection);

    // 连接设备管理器信号到数据处理器（使用Qt::QueuedConnection确保线程安全）
    if (m_deviceManager) {
        connect(m_deviceManager, &Device::DeviceManager::rawDataPointReady,
                m_dataProcessor, &Processing::DataProcessor::onRawDataPointReceived, Qt::QueuedConnection);
        connect(m_deviceManager, &Device::DeviceManager::deviceStatusChanged,
                m_dataProcessor, &Processing::DataProcessor::onDeviceStatusChanged, Qt::QueuedConnection);
    }

    // 启动处理器线程
    m_processorThread->start();

    // 创建通道
    if (m_configManager) {
        QMap<QString, Core::ChannelConfig> channelConfigs = m_configManager->getChannelConfigs();
        if (!channelConfigs.isEmpty()) {
            // 使用QMetaObject::invokeMethod确保在正确的线程中创建通道
            bool success = false;
            QMetaObject::invokeMethod(m_dataProcessor, [this, channelConfigs, &success]() {
                success = m_dataProcessor->createChannels(channelConfigs);
            }, Qt::BlockingQueuedConnection);

            if (success) {
                // qDebug() << "成功创建" << channelConfigs.size() << "个通道";
            } else {
                // qDebug() << "创建通道失败!";
            }
        } else {
            // qDebug() << "没有通道配置";
        }
    }

    // qDebug() << "初始化处理模块完成，主线程ID:" << QThread::currentThreadId();
}

void MainWindow::testDataSynchronizer()
{
    if (!m_dataProcessor) {
        // qDebug() << "数据处理器未初始化!";
        return;
    }

    // 获取同步间隔
    int syncInterval = m_dataProcessor->getSyncIntervalMs();
    // qDebug() << "数据处理间隔:" << syncInterval << "毫秒";

    // 获取通道（使用QMetaObject::invokeMethod确保在正确的线程中获取通道）
    QMap<QString, Processing::Channel*> channels;
    QMetaObject::invokeMethod(m_dataProcessor, [this, &channels]() {
        channels = m_dataProcessor->getChannels();
    }, Qt::BlockingQueuedConnection);

    // qDebug() << "通道数量:" << channels.size();

    // 打印每个通道的信息
    for (auto it = channels.constBegin(); it != channels.constEnd(); ++it) {
        // qDebug() << "通道ID:" << it.key();
        // qDebug() << "  通道名称:" << it.value()->getChannelName();
        // qDebug() << "  设备ID:" << it.value()->getDeviceId();
        // qDebug() << "  硬件通道:" << it.value()->getHardwareChannel();
        // qDebug() << "  状态:" << Core::statusCodeToString(it.value()->getStatus());
    }

    // 启动处理
    QMetaObject::invokeMethod(m_dataProcessor, &Processing::DataProcessor::startProcessing, Qt::QueuedConnection);
    // qDebug() << "启动数据处理";
}

void MainWindow::onRawDataPointReady(QString deviceId, QString hardwareChannel, double rawValue, qint64 timestamp)
{
    // 将时间戳转换为可读格式
    QString timeStr = QDateTime::fromMSecsSinceEpoch(timestamp).toString("hh:mm:ss.zzz");

    // 输出原始数据点信息
     qDebug() << "原始数据点 [" << timeStr << "] 设备:" << deviceId
             << "通道:" << hardwareChannel << "值:" << rawValue;
}

void MainWindow::onDeviceStatusChanged(QString deviceId, Core::StatusCode status, QString message)
{
    // 输出设备状态变化信息
     qDebug() << "设备状态变化 - 设备:" << deviceId
             << "状态:" << Core::statusCodeToString(status)
             << "消息:" << message;
}

void MainWindow::onErrorOccurred(QString deviceId, QString errorMsg)
{
    // 输出设备错误信息
    // qDebug() << "设备错误 - 设备:" << deviceId << "错误:" << errorMsg;
}

void MainWindow::onProcessedDataPointReady(QString channelId, Core::ProcessedDataPoint dataPoint)
{
    // 将时间戳转换为可读格式
    QString timeStr = QDateTime::fromMSecsSinceEpoch(dataPoint.timestamp).toString("hh:mm:ss.zzz");

    // 输出处理后数据点信息
     qDebug() << "处理后数据点 [" << timeStr << "] 通道:" << channelId
             << "值:" << dataPoint.value << dataPoint.unit
             << "状态:" << Core::statusCodeToString(dataPoint.status);
}


void MainWindow::onChannelStatusChanged(QString channelId, Core::StatusCode status, QString message)
{
    // 输出通道状态变化信息
     qDebug() << "通道状态变化 - 通道:" << channelId
             << "状态:" << Core::statusCodeToString(status)
             << "消息:" << message;
}

void MainWindow::onProcessingError(QString errorMsg)
{
    // 输出处理错误信息
    qDebug() << "处理错误:" << errorMsg;
}

void MainWindow::onStorageStatusChanged(bool isStoraging, QString filePath)
{
    // 输出存储状态变化信息
    qDebug() << "存储状态变化 - 是否正在存储:" << isStoraging
             << "文件路径:" << filePath;
}

void MainWindow::onStorageError(QString errorMsg)
{
    // 输出存储错误信息
    qDebug() << "存储错误:" << errorMsg;
}

void MainWindow::initializeUI()
{
    // 设置窗口标题
    setWindowTitle("数据采集系统");

    // 创建中央部件和布局
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 创建控制面板
    QHBoxLayout* controlLayout = new QHBoxLayout();

    // 创建开始/停止按钮
    m_startStopButton = new QPushButton("开始采集", this);
    connect(m_startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopButtonClicked);
    controlLayout->addWidget(m_startStopButton);

    // 添加弹簧
    controlLayout->addStretch();

    // 添加控制面板到主布局
    mainLayout->addLayout(controlLayout);

    // 创建图表
    m_plot = new QCustomPlot(this);
    mainLayout->addWidget(m_plot);

    // 设置中央部件
    setCentralWidget(centralWidget);

    // 设置图表
    setupPlot();

    // 创建图表更新定时器
    m_plotUpdateTimer = new QTimer(this);
    connect(m_plotUpdateTimer, &QTimer::timeout, this, &MainWindow::updatePlot);
    m_plotUpdateTimer->setInterval(50); // 50ms更新一次图表，提高刷新率
}

void MainWindow::setupPlot()
{
    // 设置图表标题
    m_plot->plotLayout()->insertRow(0);
    m_plot->plotLayout()->addElement(0, 0, new QCPTextElement(m_plot, "通道数据实时显示", QFont("sans", 12, QFont::Bold)));

    // 设置坐标轴标签
    m_plot->xAxis->setLabel("时间 (秒)");
    m_plot->yAxis->setLabel("值");

    // 设置图例可见
    m_plot->legend->setVisible(true);

    // 允许用户交互
    m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    // 设置坐标轴范围
    m_plot->xAxis->setRange(0, m_timeWindow);
    m_plot->yAxis->setRange(-10, 10);

    // 性能优化设置
    m_plot->setNoAntialiasingOnDrag(true);  // 拖动时禁用抗锯齿以提高性能
    m_plot->setNotAntialiasedElements(QCP::aeAll); // 禁用所有元素的抗锯齿
    m_plot->setPlottingHints(QCP::phFastPolylines | QCP::phImmediateRefresh); // 设置绘图提示

    // 优化刷新
    m_plot->replot(QCustomPlot::rpQueuedReplot);

    // 添加通道到图表
    if (m_dataProcessor) {
        // 获取通道（使用QMetaObject::invokeMethod确保在正确的线程中获取通道）
        QMap<QString, Processing::Channel*> channels;
        QMetaObject::invokeMethod(m_dataProcessor, [this, &channels]() {
            channels = m_dataProcessor->getChannels();
        }, Qt::BlockingQueuedConnection);

        // 定义一些颜色
        QVector<QColor> colors = {
            QColor(255, 0, 0),      // 红色
            QColor(0, 0, 255),      // 蓝色
            QColor(0, 128, 0),      // 绿色
            QColor(128, 0, 128),    // 紫色
            QColor(255, 165, 0),    // 橙色
            QColor(0, 128, 128),    // 青色
            QColor(128, 0, 0),      // 深红色
            QColor(0, 0, 128)       // 深蓝色
        };

        int colorIndex = 0;
        for (auto it = channels.constBegin(); it != channels.constEnd(); ++it) {
            QString channelId = it.key();
            QString channelName = it.value()->getChannelName();
            QColor color = colors[colorIndex % colors.size()];

            addChannelToPlot(channelId, channelName, color);
            colorIndex++;
        }
    }
}

void MainWindow::addChannelToPlot(const QString& channelId, const QString& channelName, const QColor& color)
{
    // 创建新的图表
    QCPGraph* graph = m_plot->addGraph();
    graph->setName(channelName);

    // 设置图表样式
    QPen pen;
    pen.setColor(color);
    pen.setWidth(2);
    graph->setPen(pen);

    // 优化性能设置
    graph->setAdaptiveSampling(true);  // 启用自适应采样
    graph->setLineStyle(QCPGraph::lsLine); // 线型
    graph->setScatterStyle(QCPScatterStyle::ssNone); // 不显示散点，提高性能

    // 存储图表对象
    m_channelGraphs[channelId] = graph;

    // 重绘图表
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void MainWindow::onStartStopButtonClicked()
{
    if (!m_dataProcessor || !m_deviceManager) {
        // qDebug() << "数据处理器或设备管理器未初始化!";
        return;
    }

    if (m_isAcquiring) {
        // 停止采集
        m_deviceManager->stopAllDevices();
        QMetaObject::invokeMethod(m_dataProcessor, &Processing::DataProcessor::stopProcessing, Qt::QueuedConnection);

        // 停止数据存储
        QMetaObject::invokeMethod(m_dataProcessor, &Processing::DataProcessor::stopDataStorage, Qt::QueuedConnection);

        m_plotUpdateTimer->stop();

        m_startStopButton->setText("开始采集");
        m_isAcquiring = false;

        // qDebug() << "停止数据采集";
    } else {
        // 开始采集
        // 清除所有通道的数据
        for (auto it = m_channelGraphs.begin(); it != m_channelGraphs.end(); ++it) {
            it.value()->data()->clear();
        }

        // 清除处理器中的数据缓冲区
        QMetaObject::invokeMethod(m_dataProcessor, &Processing::DataProcessor::clearAllBuffers, Qt::QueuedConnection);

        // 记录开始时间戳
        m_startTimestamp = QDateTime::currentMSecsSinceEpoch();

        // 启动设备和处理
        m_deviceManager->startAllDevices();
        QMetaObject::invokeMethod(m_dataProcessor, &Processing::DataProcessor::startProcessing, Qt::QueuedConnection);

        // 开始数据存储
        QMetaObject::invokeMethod(m_dataProcessor,
            [this]() {
                m_dataProcessor->startDataStorage(m_startTimestamp);
            }, Qt::QueuedConnection);

        m_plotUpdateTimer->start();

        m_startStopButton->setText("停止采集");
        m_isAcquiring = true;

        // qDebug() << "开始数据采集";
    }
}

void MainWindow::updatePlot()
{
    if (!m_dataProcessor || !m_isAcquiring) {
        return;
    }

    // 获取当前时间
    double currentTime = (QDateTime::currentMSecsSinceEpoch() - m_startTimestamp) / 1000.0;

    // 获取所有通道的最新数据点
    QMap<QString, QPair<double, double>> latestPoints;
    QMetaObject::invokeMethod(m_dataProcessor, [this, &latestPoints]() {
        latestPoints = m_dataProcessor->getLatestDataPoints();
    }, Qt::BlockingQueuedConnection);

    // 更新每个通道的图表
    for (auto it = m_channelGraphs.begin(); it != m_channelGraphs.end(); ++it) {
        QString channelId = it.key();
        QCPGraph* graph = it.value();

        if (latestPoints.contains(channelId)) {
            // 获取最新数据点
            double timestamp = latestPoints[channelId].first;
            double value = latestPoints[channelId].second;

            // 计算相对时间戳（秒）
            double relativeTime = (timestamp - m_startTimestamp) / 1000.0;

            // 添加数据点（使用addData而不是setData）
            graph->addData(relativeTime, value);

            // 限制数据点数量
            // 如果数据点超过显示限制，移除旧数据点
            if (graph->data()->size() > m_displayPointCount) {
                // 计算需要保留的最早键值（时间戳）
                double key = relativeTime - m_timeWindow;
                graph->data()->removeBefore(key);
            }

            // 自动调整X轴范围以显示最新数据
            double keyRange = qMin(m_timeWindow, currentTime);
            m_plot->xAxis->setRange(currentTime - keyRange, currentTime);
        }
    }

    // 自动调整Y轴范围
    m_plot->rescaleAxes();

    // 重绘图表
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void MainWindow::onSyncFrameReady(Core::SynchronizedDataFrame frame)
{
    // 将时间戳转换为可读格式
    QString timeStr = frame.getFormattedTimestamp();

    // 计算相对时间（秒）
    double relativeTime = (frame.timestamp - m_startTimestamp) / 1000.0;

    // 输出同步数据帧信息
    // qDebug() << "同步数据帧 [" << timeStr << "] 通道数量:" << frame.channelData.size();

    // 注意：不再在这里更新图表，而是在updatePlot方法中统一更新
    // 这样可以减少UI线程的负担，提高性能
}
