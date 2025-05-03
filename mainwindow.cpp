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
#include <QResizeEvent>
#include <QSplitterHandle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_configManager(nullptr)
    , m_deviceManager(nullptr)
    , m_dataProcessor(nullptr)
    , m_processorThread(nullptr)
    , m_startStopButton(nullptr)
    , m_plot(nullptr)
    , m_dashboard1(nullptr)
    , m_dashboard2(nullptr)
    , m_dashboard3(nullptr)
    , m_dashboard4(nullptr)
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
                qDebug() << "成功创建" << channelConfigs.size() << "个通道";
            } else {
                qDebug() << "创建通道失败!";
            }
        } else {
            qDebug() << "没有通道配置";
        }

        // 创建二次计算仪器
        QList<Core::SecondaryInstrumentConfig> secondaryInstrumentConfigs = m_configManager->getSecondaryInstrumentConfigs();
        if (!secondaryInstrumentConfigs.isEmpty()) {
            // 使用QMetaObject::invokeMethod确保在正确的线程中创建二次计算仪器
            bool success = false;
            QMetaObject::invokeMethod(m_dataProcessor, [this, secondaryInstrumentConfigs, &success]() {
                success = m_dataProcessor->createSecondaryInstruments(secondaryInstrumentConfigs);
            }, Qt::BlockingQueuedConnection);

            if (success) {
                qDebug() << "成功创建" << secondaryInstrumentConfigs.size() << "个二次计算仪器";
            } else {
                qDebug() << "创建二次计算仪器失败!";
            }
        } else {
            qDebug() << "没有二次计算仪器配置";
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

    // 设置QSplitter的初始大小比例
    // 设置水平分割器（左右布局）的比例为1:1
    ui->mainSplitter->setSizes(QList<int>() << 960 << 960);

    // 设置左侧垂直分割器（上下布局）的比例为3:1
    ui->leftSplitter->setSizes(QList<int>() << 750 << 250);

    // 设置右侧垂直分割器（上下布局）的比例为3:1
    ui->rightSplitter->setSizes(QList<int>() << 750 << 250);

    // 锁定分割器，防止用户改变比例
    lockSplitters();

    // 强制设置分割器位置，确保比例正确
    QTimer::singleShot(100, this, [this]() {
        // 重新设置分割器大小，确保比例正确
        ui->mainSplitter->setSizes(QList<int>() << 960 << 960);
        ui->leftSplitter->setSizes(QList<int>() << 750 << 250);
        ui->rightSplitter->setSizes(QList<int>() << 750 << 250);

        // 输出调试信息
        qDebug() << "分割器大小已重置，主分割器大小:" << ui->mainSplitter->sizes()
                 << "，左分割器大小:" << ui->leftSplitter->sizes()
                 << "，右分割器大小:" << ui->rightSplitter->sizes();
    });

    // 创建控制面板 - 放在optionLayout中
    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->setContentsMargins(4, 4, 4, 4);

    // 创建开始/停止按钮
    m_startStopButton = new QPushButton("开始采集", this);
    connect(m_startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopButtonClicked);
    controlLayout->addWidget(m_startStopButton);

    // 添加弹簧
    controlLayout->addStretch();

    // 将控制面板添加到optionGroupBox
    QWidget* optionControlWidget = new QWidget();
    optionControlWidget->setLayout(controlLayout);
    ui->optionGroupBox->layout()->addWidget(optionControlWidget);

    // 设置optionGroupBox的大小策略
    ui->optionGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // 创建图表并添加到plotGroupBox
    m_plot = new QCustomPlot(this);

    // 设置QCustomPlot的大小策略，使其能够动态填充plotGroupBox
    m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 将QCustomPlot添加到plotGroupBox的布局中
    ui->plotGroupBox->layout()->addWidget(m_plot);

    // 确保QCustomPlot能够正确显示
    m_plot->setMinimumHeight(700);

    // 设置plotGroupBox的大小策略
    ui->plotGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 查找主要采集通道
    m_mainChannels = findMainAcquisitionChannels();

    // 设置图表
    setupPlot();

    // 设置仪表盘
    setupDashboards();

    // 设置仪表
    setupInstruments();

    // 创建图表更新定时器
    m_plotUpdateTimer = new QTimer(this);
    connect(m_plotUpdateTimer, &QTimer::timeout, this, &MainWindow::updatePlot);
    m_plotUpdateTimer->setInterval(50); // 50ms更新一次图表，提高刷新率

    // 输出调试信息
    qDebug() << "UI初始化完成，主分割器大小:" << ui->mainSplitter->sizes()
             << "，左分割器大小:" << ui->leftSplitter->sizes()
             << "，右分割器大小:" << ui->rightSplitter->sizes();
}

void MainWindow::setupPlot()
{
    // 设置图表标题
    m_plot->plotLayout()->insertRow(0);
    m_plot->plotLayout()->addElement(0, 0, new QCPTextElement(m_plot, "主要采集量实时显示", QFont("sans", 12, QFont::Bold)));

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

    // 设置自动调整大小
    m_plot->plotLayout()->setAutoMargins(QCP::msAll); // 自动调整所有边距

    // 设置图表布局填充整个可用空间
    QCPLayoutGrid* plotLayout = m_plot->plotLayout();
    plotLayout->setRowStretchFactor(1, 1); // 让图表区域可以拉伸
    plotLayout->setColumnStretchFactor(0, 1);

    // 优化刷新
    m_plot->replot(QCustomPlot::rpQueuedReplot);

    // 添加主要采集量通道到图表
    if (m_dataProcessor && !m_mainChannels.isEmpty()) {
        // 定义一些颜色
        QVector<QColor> colors = {
            QColor(255, 0, 0),      // 红色 - 节气门位置
            QColor(0, 0, 255),      // 蓝色 - 发动机转速
            QColor(0, 128, 0),      // 绿色 - 发动机力矩
            QColor(128, 0, 128)     // 紫色 - 发动机功率
        };

        // 定义主要采集量类型和对应的颜色索引
        QMap<QString, int> typeColorIndex = {
            {"throttle_position", 0},
            {"engine_speed", 1},
            {"engine_force", 2},
            {"engine_power", 3}
        };

        // 获取所有通道（使用QMetaObject::invokeMethod确保在正确的线程中获取通道）
        QMap<QString, Processing::Channel*> channels;
        QMetaObject::invokeMethod(m_dataProcessor, [this, &channels]() {
            channels = m_dataProcessor->getChannels();
        }, Qt::BlockingQueuedConnection);

        // 获取二次计算仪器通道
        QMap<QString, Processing::SecondaryInstrument*> secondaryInstruments;
        QMetaObject::invokeMethod(m_dataProcessor, [this, &secondaryInstruments]() {
            secondaryInstruments = m_dataProcessor->getSecondaryInstruments();
        }, Qt::BlockingQueuedConnection);

        // 添加主要采集量通道到图表
        for (auto it = m_mainChannels.constBegin(); it != m_mainChannels.constEnd(); ++it) {
            QString acquisitionType = it.key();
            QString channelId = it.value();

            // 获取颜色索引
            int colorIndex = typeColorIndex.value(acquisitionType, 0);
            QColor color = colors[colorIndex % colors.size()];

            // 获取通道名称
            QString channelName;

            // 检查是否是主通道
            if (channels.contains(channelId)) {
                channelName = channels[channelId]->getChannelName();

                // 获取通道配置
                Core::ChannelConfig channelConfig = m_configManager->getChannelConfigs().value(channelId);

                // 输出显示格式信息
                qDebug() << "添加主要采集量通道到图表:" << channelId
                         << "中文标签=" << channelConfig.displayFormat.labelInChinese
                         << "采集类型=" << channelConfig.displayFormat.acquisitionType
                         << "单位=" << channelConfig.displayFormat.unit;

                // 添加通道到图表
                addChannelToPlot(channelId, channelName, color);
            }
            // 检查是否是二次计算仪器通道
            else {
                // 查找二次计算仪器
                for (auto instIt = secondaryInstruments.constBegin(); instIt != secondaryInstruments.constEnd(); ++instIt) {
                    if (instIt.key() == channelId) {
                        channelName = instIt.value()->getChannelName();

                        // 获取二次计算仪器配置
                        Core::SecondaryInstrumentConfig instrumentConfig;
                        for (const auto& config : m_configManager->getSecondaryInstrumentConfigs()) {
                            if (config.channelName == channelName) {
                                instrumentConfig = config;
                                break;
                            }
                        }

                        // 输出显示格式信息
                        qDebug() << "添加主要采集量二次计算仪器通道到图表:" << channelId << channelName
                                 << "中文标签=" << instrumentConfig.displayFormat.labelInChinese
                                 << "采集类型=" << instrumentConfig.displayFormat.acquisitionType
                                 << "单位=" << instrumentConfig.displayFormat.unit;

                        // 添加二次计算仪器通道到图表
                        addChannelToPlot(channelId, channelName, color);
                        break;
                    }
                }
            }
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

void MainWindow::setupDashboards()
{
    // 检查主要采集量通道是否已找到
    if (m_mainChannels.isEmpty()) {
        qDebug() << "未找到主要采集量通道，无法设置仪表盘";
        return;
    }

    // 定义主要采集量类型和对应的仪表盘索引
    QMap<QString, int> typeDashboardIndex = {
        {"throttle_position", 0}, // 节气门位置 -> dashWidget1
        {"engine_speed", 1},      // 发动机转速 -> dashWidget2
        {"engine_force", 2},      // 发动机力矩 -> dashWidget3
        {"engine_power", 3}       // 发动机功率 -> dashWidget4
    };

    // 获取所有通道配置
    QMap<QString, Core::ChannelConfig> channelConfigs = m_configManager->getChannelConfigs();

    // 获取二次计算仪器配置
    QList<Core::SecondaryInstrumentConfig> secondaryInstrumentConfigs = m_configManager->getSecondaryInstrumentConfigs();

    // 创建仪表盘
    QList<QWidget*> dashWidgets = {
        ui->dashWidget1,
        ui->dashWidget2,
        ui->dashWidget3,
        ui->dashWidget4
    };

    // 遍历主要采集量通道
    for (auto it = m_mainChannels.constBegin(); it != m_mainChannels.constEnd(); ++it) {
        QString acquisitionType = it.key();
        QString channelId = it.value();

        // 获取仪表盘索引
        int dashboardIndex = typeDashboardIndex.value(acquisitionType, -1);
        if (dashboardIndex < 0 || dashboardIndex >= dashWidgets.size()) {
            qDebug() << "无效的仪表盘索引:" << dashboardIndex << "，采集类型:" << acquisitionType;
            continue;
        }

        // 获取显示格式参数
        Core::DisplayFormat displayFormat;

        // 检查是否是主通道
        if (channelConfigs.contains(channelId)) {
            displayFormat = channelConfigs[channelId].displayFormat;
        }
        // 检查是否是二次计算仪器通道
        else {
            for (const auto& config : secondaryInstrumentConfigs) {
                if (config.channelName == channelId) {
                    displayFormat = config.displayFormat;
                    break;
                }
            }
        }

        // 创建仪表盘
        QVBoxLayout* layout = new QVBoxLayout(dashWidgets[dashboardIndex]);
        layout->setContentsMargins(0, 0, 0, 0);

        // 创建Dashboard实例
        Dashboard* dashboard = new Dashboard(dashWidgets[dashboardIndex]);

        // 配置仪表盘
        dashboard->configure(
            displayFormat.labelInChinese,
            displayFormat.unit,
            displayFormat.resolution,
            displayFormat.minRange,
            displayFormat.maxRange
        );

        // 设置仪表盘样式
        dashboard->setPointerColor(QColor(200, 0, 0));
        dashboard->setScaleColor(Qt::black);
        dashboard->setTextColor(Qt::black);
        dashboard->setForegroundColor(QColor(50, 50, 50));
        dashboard->setAnimationEnabled(true);

        // 将仪表盘添加到布局
        layout->addWidget(dashboard);

        // 存储仪表盘
        switch (dashboardIndex) {
            case 0: m_dashboard1 = dashboard; break;
            case 1: m_dashboard2 = dashboard; break;
            case 2: m_dashboard3 = dashboard; break;
            case 3: m_dashboard4 = dashboard; break;
        }

        // 输出调试信息
        qDebug() << "创建仪表盘:" << dashboardIndex
                 << "，采集类型:" << acquisitionType
                 << "，通道ID:" << channelId
                 << "，中文标签:" << displayFormat.labelInChinese
                 << "，单位:" << displayFormat.unit
                 << "，分辨率:" << displayFormat.resolution
                 << "，最小值:" << displayFormat.minRange
                 << "，最大值:" << displayFormat.maxRange;
    }
}

void MainWindow::setupInstruments()
{
    // 按采集类型分类通道
    QMap<QString, QList<QString>> channelsByType = classifyChannelsByAcquisitionType();

    // 如果没有通道，直接返回
    if (channelsByType.isEmpty()) {
        qDebug() << "未找到通道，无法设置仪表";
        return;
    }

    // 创建柱状仪表
    createColumnarInstruments(channelsByType);
}

void MainWindow::createColumnarInstruments(const QMap<QString, QList<QString>>& channelsByType)
{
    // 获取所有通道配置
    QMap<QString, Core::ChannelConfig> channelConfigs = m_configManager->getChannelConfigs();

    // 获取二次计算仪器配置
    QList<Core::SecondaryInstrumentConfig> secondaryInstrumentConfigs = m_configManager->getSecondaryInstrumentConfigs();

    // 创建仪表布局
    QVBoxLayout* instrumentLayout = new QVBoxLayout();
    instrumentLayout->setContentsMargins(0, 0, 0, 0);
    instrumentLayout->setSpacing(8);

    // 遍历每种采集类型
    for (auto it = channelsByType.constBegin(); it != channelsByType.constEnd(); ++it) {
        QString acquisitionType = it.key();
        QList<QString> channelIds = it.value();

        // 跳过主要采集量类型（已经在仪表盘中显示）
        if (m_mainChannels.contains(acquisitionType)) {
            continue;
        }

        // 创建该采集类型的行布局
        QHBoxLayout* rowLayout = new QHBoxLayout();
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);

        // 遍历该采集类型的所有通道
        for (const QString& channelId : channelIds) {
            // 获取显示格式参数
            Core::DisplayFormat displayFormat;

            // 检查是否是主通道
            if (channelConfigs.contains(channelId)) {
                displayFormat = channelConfigs[channelId].displayFormat;
            }
            // 检查是否是二次计算仪器通道
            else {
                for (const auto& config : secondaryInstrumentConfigs) {
                    if (config.channelName == channelId) {
                        displayFormat = config.displayFormat;
                        break;
                    }
                }
            }

            // 创建柱状仪表
            ColumnarInstrument* instrument = new ColumnarInstrument();

            // 配置仪表
            instrument->configure(
                displayFormat.labelInChinese,
                displayFormat.unit,
                displayFormat.resolution,
                displayFormat.minRange,
                displayFormat.maxRange
            );

            // 将仪表添加到行布局
            rowLayout->addWidget(instrument);

            // 存储仪表
            m_columnarInstruments[acquisitionType].append(instrument);

            // 输出调试信息
            qDebug() << "创建柱状仪表:"
                     << "，采集类型:" << acquisitionType
                     << "，通道ID:" << channelId
                     << "，中文标签:" << displayFormat.labelInChinese
                     << "，单位:" << displayFormat.unit
                     << "，分辨率:" << displayFormat.resolution
                     << "，最小值:" << displayFormat.minRange
                     << "，最大值:" << displayFormat.maxRange;
        }

        // 添加弹簧，确保仪表均匀分布
        rowLayout->addStretch();

        // 将行布局添加到主布局
        instrumentLayout->addLayout(rowLayout);
    }

    // 添加弹簧，确保行布局向上对齐
    instrumentLayout->addStretch();

    // 将主布局添加到instrumentGroupBox
    QWidget* instrumentWidget = new QWidget();
    instrumentWidget->setLayout(instrumentLayout);
    ui->instrumentGroupBox->layout()->addWidget(instrumentWidget);
}

void MainWindow::updateDashboards()
{
    if (!m_dataProcessor || !m_isAcquiring) {
        return;
    }

    // 获取所有通道的最新数据点
    QMap<QString, QPair<double, double>> latestPoints;
    QMetaObject::invokeMethod(m_dataProcessor, [this, &latestPoints]() {
        latestPoints = m_dataProcessor->getLatestDataPoints();
    }, Qt::BlockingQueuedConnection);

    // 更新仪表盘
    for (auto it = m_mainChannels.constBegin(); it != m_mainChannels.constEnd(); ++it) {
        QString acquisitionType = it.key();
        QString channelId = it.value();

        if (latestPoints.contains(channelId)) {
            double value = latestPoints[channelId].second;

            // 根据采集类型更新对应的仪表盘
            if (acquisitionType == "throttle_position" && m_dashboard1) {
                m_dashboard1->setValue(value);
            }
            else if (acquisitionType == "engine_speed" && m_dashboard2) {
                m_dashboard2->setValue(value);
            }
            else if (acquisitionType == "engine_force" && m_dashboard3) {
                m_dashboard3->setValue(value);
            }
            else if (acquisitionType == "engine_power" && m_dashboard4) {
                m_dashboard4->setValue(value);
            }
        }
    }
}

void MainWindow::updateInstruments()
{
    if (!m_dataProcessor || !m_isAcquiring) {
        return;
    }

    // 获取所有通道的最新数据点
    QMap<QString, QPair<double, double>> latestPoints;
    QMetaObject::invokeMethod(m_dataProcessor, [this, &latestPoints]() {
        latestPoints = m_dataProcessor->getLatestDataPoints();
    }, Qt::BlockingQueuedConnection);

    // 按采集类型分类通道
    QMap<QString, QList<QString>> channelsByType = classifyChannelsByAcquisitionType();

    // 更新柱状仪表
    for (auto typeIt = channelsByType.constBegin(); typeIt != channelsByType.constEnd(); ++typeIt) {
        QString acquisitionType = typeIt.key();
        QList<QString> channelIds = typeIt.value();

        // 跳过主要采集量类型（已经在仪表盘中显示）
        if (m_mainChannels.contains(acquisitionType)) {
            continue;
        }

        // 检查该采集类型是否有仪表
        if (!m_columnarInstruments.contains(acquisitionType) || m_columnarInstruments[acquisitionType].isEmpty()) {
            continue;
        }

        // 获取该采集类型的仪表列表
        QList<ColumnarInstrument*> instruments = m_columnarInstruments[acquisitionType];

        // 遍历该采集类型的所有通道
        for (int i = 0; i < channelIds.size() && i < instruments.size(); ++i) {
            QString channelId = channelIds[i];
            ColumnarInstrument* instrument = instruments[i];

            if (latestPoints.contains(channelId)) {
                double value = latestPoints[channelId].second;
                instrument->setValue(value);
            }
        }
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

    // 更新仪表盘
    updateDashboards();

    // 更新仪表
    updateInstruments();
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
    // 调用父类的resizeEvent
    QMainWindow::resizeEvent(event);

    // 保持分割器的比例
    QSize newSize = event->size();

    // 计算新的分割器大小
    int halfWidth = newSize.width() / 2;

    // 重新设置主分割器的比例为1:1
    ui->mainSplitter->setSizes(QList<int>() << halfWidth << halfWidth);

    // 计算左右两侧的高度（减去菜单栏和状态栏的高度）
    int availableHeight = newSize.height() - ui->menubar->height() - ui->statusbar->height();
    int topHeight = (availableHeight * 3) / 4;
    int bottomHeight = (availableHeight * 1) / 4;

    // 重新设置左侧分割器的比例为3:1
    ui->leftSplitter->setSizes(QList<int>() << topHeight << bottomHeight);

    // 重新设置右侧分割器的比例为3:1
    ui->rightSplitter->setSizes(QList<int>() << topHeight << bottomHeight);

    // 确保分割器保持锁定
    lockSplitters();

    // 如果图表存在，重新绘制以适应新的大小
    if (m_plot) {
        // 重新绘制图表
        m_plot->replot(QCustomPlot::rpQueuedReplot);

        // 输出调试信息
        qDebug() << "窗口大小已调整，新大小:" << event->size()
                 << "，可用高度:" << availableHeight
                 << "，上部高度:" << topHeight
                 << "，下部高度:" << bottomHeight
                 << "，图表大小:" << m_plot->size()
                 << "，主分割器大小:" << ui->mainSplitter->sizes()
                 << "，左分割器大小:" << ui->leftSplitter->sizes()
                 << "，右分割器大小:" << ui->rightSplitter->sizes();
    }
}

QMap<QString, QList<QString>> MainWindow::classifyChannelsByAcquisitionType()
{
    QMap<QString, QList<QString>> channelsByType;

    // 获取所有通道配置
    QMap<QString, Core::ChannelConfig> channelConfigs = m_configManager->getChannelConfigs();

    // 遍历所有通道配置，按采集类型分类
    for (auto it = channelConfigs.constBegin(); it != channelConfigs.constEnd(); ++it) {
        QString channelId = it.key();
        QString acquisitionType = it.value().displayFormat.acquisitionType;

        // 如果采集类型不为空，添加到对应的列表中
        if (!acquisitionType.isEmpty()) {
            channelsByType[acquisitionType].append(channelId);
        }
    }

    // 获取二次计算仪器配置
    QList<Core::SecondaryInstrumentConfig> secondaryInstrumentConfigs = m_configManager->getSecondaryInstrumentConfigs();

    // 遍历所有二次计算仪器配置，按采集类型分类
    for (const auto& config : secondaryInstrumentConfigs) {
        QString channelName = config.channelName;
        QString acquisitionType = config.displayFormat.acquisitionType;

        // 如果采集类型不为空，添加到对应的列表中
        if (!acquisitionType.isEmpty()) {
            // 使用通道名称作为ID（二次计算仪器的ID与名称相同）
            channelsByType[acquisitionType].append(channelName);
        }
    }

    // 输出调试信息
    qDebug() << "按采集类型分类的通道:";
    for (auto it = channelsByType.constBegin(); it != channelsByType.constEnd(); ++it) {
        qDebug() << "  采集类型:" << it.key() << "，通道数量:" << it.value().size() << "，通道列表:" << it.value();
    }

    return channelsByType;
}

QMap<QString, QString> MainWindow::findMainAcquisitionChannels()
{
    QMap<QString, QString> mainChannels;

    // 定义主要采集量类型
    QStringList mainTypes = {"throttle_position", "engine_speed", "engine_force", "engine_power"};

    // 获取所有通道配置
    QMap<QString, Core::ChannelConfig> channelConfigs = m_configManager->getChannelConfigs();

    // 遍历所有通道配置，查找主要采集量
    for (auto it = channelConfigs.constBegin(); it != channelConfigs.constEnd(); ++it) {
        QString channelId = it.key();
        QString acquisitionType = it.value().displayFormat.acquisitionType;

        // 如果是主要采集量类型，添加到结果中
        if (mainTypes.contains(acquisitionType) && !mainChannels.contains(acquisitionType)) {
            mainChannels[acquisitionType] = channelId;
        }
    }

    // 获取二次计算仪器配置
    QList<Core::SecondaryInstrumentConfig> secondaryInstrumentConfigs = m_configManager->getSecondaryInstrumentConfigs();

    // 遍历所有二次计算仪器配置，查找主要采集量
    for (const auto& config : secondaryInstrumentConfigs) {
        QString channelName = config.channelName;
        QString acquisitionType = config.displayFormat.acquisitionType;

        // 如果是主要采集量类型，添加到结果中
        if (mainTypes.contains(acquisitionType) && !mainChannels.contains(acquisitionType)) {
            // 使用通道名称作为ID（二次计算仪器的ID与名称相同）
            mainChannels[acquisitionType] = channelName;
        }
    }

    // 输出调试信息
    qDebug() << "主要采集量通道:";
    for (auto it = mainChannels.constBegin(); it != mainChannels.constEnd(); ++it) {
        qDebug() << "  采集类型:" << it.key() << "，通道ID:" << it.value();
    }

    return mainChannels;
}

void MainWindow::lockSplitters()
{
    // 禁止用户移动分割器
    ui->mainSplitter->setHandleWidth(1);  // 设置为1，保持可见但最小化
    ui->leftSplitter->setHandleWidth(1);
    ui->rightSplitter->setHandleWidth(1);

    // 禁用分割器交互
    ui->mainSplitter->setChildrenCollapsible(false);
    ui->leftSplitter->setChildrenCollapsible(false);
    ui->rightSplitter->setChildrenCollapsible(false);

    // 设置不透明调整，提高性能
    ui->mainSplitter->setOpaqueResize(false);
    ui->leftSplitter->setOpaqueResize(false);
    ui->rightSplitter->setOpaqueResize(false);

    // 禁用分割器移动
    for (auto handle : ui->mainSplitter->findChildren<QSplitterHandle*>()) {
        handle->setEnabled(false);
    }
    for (auto handle : ui->leftSplitter->findChildren<QSplitterHandle*>()) {
        handle->setEnabled(false);
    }
    for (auto handle : ui->rightSplitter->findChildren<QSplitterHandle*>()) {
        handle->setEnabled(false);
    }
}
