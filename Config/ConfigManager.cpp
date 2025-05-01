#include "ConfigManager.h"

namespace Config {

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_synchronizationIntervalMs(Core::DEFAULT_SYNC_INTERVAL_MS)
{
}

bool ConfigManager::loadConfig(const QString& filePath)
{
    // 保存配置文件路径
    m_configFilePath = filePath;

    // 打开并读取配置文件
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开配置文件:" << filePath << "错误:" << file.errorString();
        return false;
    }

    // 读取文件内容
    QByteArray jsonData = file.readAll();
    file.close();

    // 解析JSON
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << parseError.errorString() << "在偏移量:" << parseError.offset;
        return false;
    }

    // 检查JSON文档是否为对象
    if (!jsonDoc.isObject()) {
        qDebug() << "JSON文档不是一个对象";
        return false;
    }

    // 获取根对象
    QJsonObject rootObj = jsonDoc.object();

    // 清除之前的配置
    m_virtualDeviceConfigs.clear();
    m_modbusDeviceConfigs.clear();
    m_daqDeviceConfigs.clear();
    m_ecuDeviceConfigs.clear();
    m_channelConfigs.clear();

    // 解析同步间隔（如果存在）
    if (rootObj.contains("synchronization_interval_ms")) {
        m_synchronizationIntervalMs = rootObj["synchronization_interval_ms"].toInt(Core::DEFAULT_SYNC_INTERVAL_MS);
    } else {
        m_synchronizationIntervalMs = Core::DEFAULT_SYNC_INTERVAL_MS;
    }

    // 解析虚拟设备（目前只关注这部分）
    if (rootObj.contains("virtual_devices") && rootObj["virtual_devices"].isArray()) {
        parseVirtualDevices(rootObj["virtual_devices"].toArray());
    }

    // 解析Modbus设备（暂时不实现，但保留接口）
    if (rootObj.contains("modbus_devices") && rootObj["modbus_devices"].isArray()) {
        parseModbusDevices(rootObj["modbus_devices"].toArray());
    }

    // 解析DAQ设备（暂时不实现，但保留接口）
    if (rootObj.contains("daq_devices") && rootObj["daq_devices"].isArray()) {
        parseDAQDevices(rootObj["daq_devices"].toArray());
    }

    // 解析ECU设备（暂时不实现，但保留接口）
    if (rootObj.contains("ecu_devices") && rootObj["ecu_devices"].isArray()) {
        parseECUDevices(rootObj["ecu_devices"].toArray());
    }

    qDebug() << "配置加载成功，虚拟设备数量:" << m_virtualDeviceConfigs.size();
    return true;
}

QList<Core::DeviceConfig*> ConfigManager::getDeviceConfigs() const
{
    QList<Core::DeviceConfig*> result;

    // 添加虚拟设备
    for (const auto& config : m_virtualDeviceConfigs) {
        result.append(new Core::VirtualDeviceConfig(config));
    }

    // 添加Modbus设备
    for (const auto& config : m_modbusDeviceConfigs) {
        result.append(new Core::ModbusDeviceConfig(config));
    }

    // 添加DAQ设备
    for (const auto& config : m_daqDeviceConfigs) {
        result.append(new Core::DAQDeviceConfig(config));
    }

    // 添加ECU设备
    for (const auto& config : m_ecuDeviceConfigs) {
        result.append(new Core::ECUDeviceConfig(config));
    }

    return result;
}

QList<Core::VirtualDeviceConfig> ConfigManager::getVirtualDeviceConfigs() const
{
    return m_virtualDeviceConfigs;
}

QList<Core::ModbusDeviceConfig> ConfigManager::getModbusDeviceConfigs() const
{
    return m_modbusDeviceConfigs;
}

QList<Core::DAQDeviceConfig> ConfigManager::getDAQDeviceConfigs() const
{
    return m_daqDeviceConfigs;
}

QList<Core::ECUDeviceConfig> ConfigManager::getECUDeviceConfigs() const
{
    return m_ecuDeviceConfigs;
}

QMap<QString, Core::ChannelConfig> ConfigManager::getChannelConfigs() const
{
    return m_channelConfigs;
}

int ConfigManager::getSynchronizationIntervalMs() const
{
    return m_synchronizationIntervalMs;
}

QString ConfigManager::getConfigFilePath() const
{
    return m_configFilePath;
}

bool ConfigManager::saveConfig(const QString& filePath) const
{
    // 创建根对象
    QJsonObject rootObj;

    // 添加同步间隔
    rootObj["synchronization_interval_ms"] = m_synchronizationIntervalMs;

    // 添加虚拟设备
    QJsonArray virtualDevicesArray;
    for (const auto& device : m_virtualDeviceConfigs) {
        QJsonObject deviceObj;
        deviceObj["instance_name"] = device.instanceName;
        deviceObj["signal_type"] = device.signalType;
        deviceObj["amplitude"] = device.amplitude;
        deviceObj["frequency"] = device.frequency;

        // 添加通道参数
        QJsonObject channelParamsObj;
        channelParamsObj["gain"] = device.channelParams.gain;
        channelParamsObj["offset"] = device.channelParams.offset;
        channelParamsObj["unit"] = device.channelParams.unit;

        // 添加校准参数
        QJsonObject calibrationParamsObj;
        calibrationParamsObj["a"] = device.channelParams.calibrationParams.a;
        calibrationParamsObj["b"] = device.channelParams.calibrationParams.b;
        calibrationParamsObj["c"] = device.channelParams.calibrationParams.c;
        calibrationParamsObj["d"] = device.channelParams.calibrationParams.d;

        channelParamsObj["calibration_params"] = calibrationParamsObj;
        deviceObj["channel_params"] = channelParamsObj;

        virtualDevicesArray.append(deviceObj);
    }
    rootObj["virtual_devices"] = virtualDevicesArray;

    // 创建JSON文档
    QJsonDocument jsonDoc(rootObj);

    // 打开文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "无法打开文件进行写入:" << filePath << "错误:" << file.errorString();
        return false;
    }

    // 写入JSON数据
    file.write(jsonDoc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "配置已保存到:" << filePath;
    return true;
}

void ConfigManager::parseVirtualDevices(const QJsonArray& jsonArray)
{
    // 遍历虚拟设备数组
    for (int i = 0; i < jsonArray.size(); ++i) {
        if (!jsonArray[i].isObject()) {
            qDebug() << "跳过非对象虚拟设备条目";
            continue;
        }

        QJsonObject deviceObj = jsonArray[i].toObject();

        // 提取必要字段
        QString instanceName = deviceObj["instance_name"].toString();
        QString signalType = deviceObj["signal_type"].toString("sine");
        double amplitude = deviceObj["amplitude"].toDouble(1.0);
        double frequency = deviceObj["frequency"].toDouble(1.0);

        // 解析通道参数
        Core::ChannelParams channelParams;
        if (deviceObj.contains("channel_params") && deviceObj["channel_params"].isObject()) {
            channelParams = parseChannelParams(deviceObj["channel_params"].toObject());
        }

        // 创建虚拟设备配置
        Core::VirtualDeviceConfig config;
        config.deviceId = instanceName; // 使用实例名称作为设备ID
        config.instanceName = instanceName;
        config.signalType = signalType;
        config.amplitude = amplitude;
        config.frequency = frequency;
        config.channelParams = channelParams;

        // 添加到列表
        m_virtualDeviceConfigs.append(config);

        // 创建对应的通道配置
        Core::ChannelConfig channelConfig;
        channelConfig.channelId = instanceName; // 使用实例名称作为通道ID
        channelConfig.channelName = instanceName;
        channelConfig.deviceId = instanceName;
        channelConfig.hardwareChannel = "0"; // 虚拟设备只有一个通道
        channelConfig.params = channelParams;

        // 添加到通道映射
        m_channelConfigs[channelConfig.channelId] = channelConfig;

        qDebug() << "已加载虚拟设备:" << instanceName << "信号类型:" << signalType
                 << "振幅:" << amplitude << "频率:" << frequency;
    }
}

void ConfigManager::parseModbusDevices(const QJsonArray& jsonArray)
{
    // 暂时只记录数量，不实际解析
    qDebug() << "发现" << jsonArray.size() << "个Modbus设备（暂不解析）";
}

void ConfigManager::parseDAQDevices(const QJsonArray& jsonArray)
{
    // 暂时只记录数量，不实际解析
    qDebug() << "发现" << jsonArray.size() << "个DAQ设备（暂不解析）";
}

void ConfigManager::parseECUDevices(const QJsonArray& jsonArray)
{
    // 暂时只记录数量，不实际解析
    qDebug() << "发现" << jsonArray.size() << "个ECU设备（暂不解析）";
}

Core::ChannelParams ConfigManager::parseChannelParams(const QJsonObject& jsonObject)
{
    Core::ChannelParams params;

    // 提取增益和偏移
    params.gain = jsonObject["gain"].toDouble(1.0);
    params.offset = jsonObject["offset"].toDouble(0.0);

    // 提取单位（如果存在）
    if (jsonObject.contains("unit")) {
        params.unit = jsonObject["unit"].toString();
    }

    // 提取校准参数
    if (jsonObject.contains("calibration_params") && jsonObject["calibration_params"].isObject()) {
        params.calibrationParams = parseCalibrationParams(jsonObject["calibration_params"].toObject());
    }

    return params;
}

Core::CalibrationParams ConfigManager::parseCalibrationParams(const QJsonObject& jsonObject)
{
    Core::CalibrationParams params;

    // 提取校准多项式系数
    params.a = jsonObject["a"].toDouble(0.0);
    params.b = jsonObject["b"].toDouble(0.0);
    params.c = jsonObject["c"].toDouble(1.0);
    params.d = jsonObject["d"].toDouble(0.0);

    return params;
}

Core::SerialConfig ConfigManager::parseSerialConfig(const QJsonObject& jsonObject)
{
    Core::SerialConfig config;

    // 提取串口配置
    config.port = jsonObject["port"].toString();
    config.baudrate = jsonObject["baudrate"].toInt(9600);
    config.databits = jsonObject["databits"].toInt(8);
    config.stopbits = jsonObject["stopbits"].toInt(1);
    config.parity = jsonObject["parity"].toString("N");

    return config;
}

} // namespace Config
