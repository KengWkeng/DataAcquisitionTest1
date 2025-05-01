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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_configManager(nullptr)
    , m_deviceManager(nullptr)
    , m_dataSynchronizer(nullptr)
{
    ui->setupUi(this);

    // 初始化配置
    initializeConfig();

    // 初始化设备
    initializeDevices();

    // 初始化处理
    initializeProcessing();

    // 测试配置管理器
    testConfigManager();

    // 测试设备管理器
    testDeviceManager();

    // 测试数据同步器
    testDataSynchronizer();
}

MainWindow::~MainWindow()
{
    // 清理数据同步器
    if (m_dataSynchronizer) {
        // 停止同步
        m_dataSynchronizer->stopSync();

        delete m_dataSynchronizer;
        m_dataSynchronizer = nullptr;
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
        qDebug() << "源配置文件存在，尝试复制到构建目录...";
        if (QFile::copy(sourceConfigPath, buildConfigPath)) {
            qDebug() << "配置文件已复制到构建目录";
        } else {
            qDebug() << "无法复制配置文件到构建目录";
        }
    }

    qDebug() << "尝试加载配置文件:" << buildConfigPath;
    bool success = m_configManager->loadConfig(buildConfigPath);

    if (!success) {
        qDebug() << "配置文件加载失败! 尝试创建简化版配置文件...";

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
            qDebug() << "简化版配置文件已创建:" << simplifiedConfigPath;

            // 尝试加载简化版配置
            success = m_configManager->loadConfig(simplifiedConfigPath);
            if (!success) {
                qDebug() << "简化版配置文件加载失败!";
            } else {
                qDebug() << "简化版配置文件加载成功!";
            }
        } else {
            qDebug() << "无法创建简化版配置文件:" << file.errorString();
        }
    } else {
        qDebug() << "配置文件加载成功!";

        // 尝试保存当前配置到新文件，以确保格式正确
        QString savedConfigPath = QCoreApplication::applicationDirPath() + "/saved_config.json";
        if (m_configManager->saveConfig(savedConfigPath)) {
            qDebug() << "当前配置已保存到:" << savedConfigPath;
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
}

void MainWindow::testConfigManager()
{
    if (!m_configManager) {
        qDebug() << "配置管理器未初始化!";
        return;
    }

    // 获取同步间隔
    int syncInterval = m_configManager->getSynchronizationIntervalMs();
    qDebug() << "同步间隔:" << syncInterval << "毫秒";

    // 获取虚拟设备配置
    QList<Core::VirtualDeviceConfig> virtualDevices = m_configManager->getVirtualDeviceConfigs();
    qDebug() << "虚拟设备数量:" << virtualDevices.size();

    // 打印每个虚拟设备的信息
    for (const auto& device : virtualDevices) {
        qDebug() << "设备ID:" << device.deviceId;
        qDebug() << "  实例名称:" << device.instanceName;
        qDebug() << "  信号类型:" << device.signalType;
        qDebug() << "  振幅:" << device.amplitude;
        qDebug() << "  频率:" << device.frequency;
        qDebug() << "  增益:" << device.channelParams.gain;
        qDebug() << "  偏移:" << device.channelParams.offset;
        qDebug() << "  校准参数:";
        qDebug() << "    a:" << device.channelParams.calibrationParams.a;
        qDebug() << "    b:" << device.channelParams.calibrationParams.b;
        qDebug() << "    c:" << device.channelParams.calibrationParams.c;
        qDebug() << "    d:" << device.channelParams.calibrationParams.d;
    }

    // 获取通道配置
    QMap<QString, Core::ChannelConfig> channelConfigs = m_configManager->getChannelConfigs();
    qDebug() << "通道数量:" << channelConfigs.size();

    // 打印每个通道的信息
    for (auto it = channelConfigs.constBegin(); it != channelConfigs.constEnd(); ++it) {
        qDebug() << "通道ID:" << it.key();
        qDebug() << "  通道名称:" << it.value().channelName;
        qDebug() << "  设备ID:" << it.value().deviceId;
        qDebug() << "  硬件通道:" << it.value().hardwareChannel;
        qDebug() << "  增益:" << it.value().params.gain;
        qDebug() << "  偏移:" << it.value().params.offset;
    }
}

void MainWindow::testDeviceManager()
{
    if (!m_deviceManager) {
        qDebug() << "设备管理器未初始化!";
        return;
    }

    // 获取所有设备
    QMap<QString, Device::AbstractDevice*> devices = m_deviceManager->getDevices();
    qDebug() << "设备数量:" << devices.size();

    // 打印每个设备的信息
    for (auto it = devices.constBegin(); it != devices.constEnd(); ++it) {
        qDebug() << "设备ID:" << it.key();
        qDebug() << "  设备类型:" << Core::deviceTypeToString(it.value()->getDeviceType());
        qDebug() << "  设备状态:" << Core::statusCodeToString(it.value()->getStatus());
    }

    // 启动所有设备
    bool success = m_deviceManager->startAllDevices();
    if (success) {
        qDebug() << "成功启动所有设备";
    } else {
        qDebug() << "启动设备失败!";
    }
}

void MainWindow::initializeProcessing()
{
    // 创建数据同步器
    int syncIntervalMs = m_configManager ? m_configManager->getSynchronizationIntervalMs() : Core::DEFAULT_SYNC_INTERVAL_MS;
    m_dataSynchronizer = new Processing::DataSynchronizer(syncIntervalMs, this);

    // 连接数据同步器信号
    connect(m_dataSynchronizer, &Processing::DataSynchronizer::processedDataPointReady,
            this, &MainWindow::onProcessedDataPointReady);
    connect(m_dataSynchronizer, &Processing::DataSynchronizer::syncFrameReady,
            this, &MainWindow::onSyncFrameReady);
    connect(m_dataSynchronizer, &Processing::DataSynchronizer::channelStatusChanged,
            this, &MainWindow::onChannelStatusChanged);
    connect(m_dataSynchronizer, &Processing::DataSynchronizer::errorOccurred,
            this, &MainWindow::onProcessingError);

    // 连接设备管理器信号到数据同步器
    if (m_deviceManager) {
        connect(m_deviceManager, &Device::DeviceManager::rawDataPointReady,
                m_dataSynchronizer, &Processing::DataSynchronizer::onRawDataPointReceived);
        connect(m_deviceManager, &Device::DeviceManager::deviceStatusChanged,
                m_dataSynchronizer, &Processing::DataSynchronizer::onDeviceStatusChanged);
    }

    // 创建通道
    if (m_configManager) {
        QMap<QString, Core::ChannelConfig> channelConfigs = m_configManager->getChannelConfigs();
        if (!channelConfigs.isEmpty()) {
            bool success = m_dataSynchronizer->createChannels(channelConfigs);
            if (success) {
                qDebug() << "成功创建" << channelConfigs.size() << "个通道";
            } else {
                qDebug() << "创建通道失败!";
            }
        } else {
            qDebug() << "没有通道配置";
        }
    }

    qDebug() << "初始化处理模块完成";
}

void MainWindow::testDataSynchronizer()
{
    if (!m_dataSynchronizer) {
        qDebug() << "数据同步器未初始化!";
        return;
    }

    // 获取同步间隔
    int syncInterval = m_dataSynchronizer->getSyncIntervalMs();
    qDebug() << "数据同步间隔:" << syncInterval << "毫秒";

    // 获取通道
    QMap<QString, Processing::Channel*> channels = m_dataSynchronizer->getChannels();
    qDebug() << "通道数量:" << channels.size();

    // 打印每个通道的信息
    for (auto it = channels.constBegin(); it != channels.constEnd(); ++it) {
        qDebug() << "通道ID:" << it.key();
        qDebug() << "  通道名称:" << it.value()->getChannelName();
        qDebug() << "  设备ID:" << it.value()->getDeviceId();
        qDebug() << "  硬件通道:" << it.value()->getHardwareChannel();
        qDebug() << "  状态:" << Core::statusCodeToString(it.value()->getStatus());
    }

    // 启动同步
    m_dataSynchronizer->startSync();
    qDebug() << "启动数据同步";
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
    qDebug() << "设备错误 - 设备:" << deviceId << "错误:" << errorMsg;
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

void MainWindow::onSyncFrameReady(Core::SynchronizedDataFrame frame)
{
    // 将时间戳转换为可读格式
    QString timeStr = frame.getFormattedTimestamp();

    // 输出同步数据帧信息
    qDebug() << "同步数据帧 [" << timeStr << "] 通道数量:" << frame.channelData.size();

    // 打印每个通道的数据
    for (auto it = frame.channelData.constBegin(); it != frame.channelData.constEnd(); ++it) {
        qDebug() << "  通道:" << it.key() << "值:" << it.value().value << it.value().unit;
    }
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
