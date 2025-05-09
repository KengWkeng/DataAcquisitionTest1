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

    // 解析ECU设备
    if (rootObj.contains("ecu_devices") && rootObj["ecu_devices"].isArray()) {
        parseECUDevices(rootObj["ecu_devices"].toArray());
    }

    // 解析二次计算仪器
    if (rootObj.contains("secondary_instruments") && rootObj["secondary_instruments"].isArray()) {
        parseSecondaryInstruments(rootObj["secondary_instruments"].toArray());
    }

    qDebug() << "配置加载成功，虚拟设备数量:" << m_virtualDeviceConfigs.size()
             << "，Modbus设备数量:" << m_modbusDeviceConfigs.size()
             << "，DAQ设备数量:" << m_daqDeviceConfigs.size()
             << "，ECU设备数量:" << m_ecuDeviceConfigs.size()
             << "，二次计算仪器数量:" << m_secondaryInstrumentConfigs.size();
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

QList<Core::SecondaryInstrumentConfig> ConfigManager::getSecondaryInstrumentConfigs() const
{
    return m_secondaryInstrumentConfigs;
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

        // 解析显示格式
        Core::DisplayFormat displayFormat;
        if (deviceObj.contains("display_format") && deviceObj["display_format"].isObject()) {
            displayFormat = parseDisplayFormat(deviceObj["display_format"].toObject());
        } else {
            // 创建默认显示格式
            displayFormat.labelInChinese = instanceName;
            displayFormat.acquisitionType = signalType;
            displayFormat.unit = channelParams.unit;
            displayFormat.resolution = 0.01;
            displayFormat.minRange = -amplitude;
            displayFormat.maxRange = amplitude;
        }

        // 创建虚拟设备配置
        Core::VirtualDeviceConfig config;
        config.deviceId = instanceName; // 使用实例名称作为设备ID
        config.instanceName = instanceName;
        config.signalType = signalType;
        config.amplitude = amplitude;
        config.frequency = frequency;
        config.channelParams = channelParams;
        config.displayFormat = displayFormat;

        // 添加到列表
        m_virtualDeviceConfigs.append(config);

        // 添加调试输出
        qDebug() << "虚拟设备显示格式:" << instanceName
                 << "中文标签=" << displayFormat.labelInChinese
                 << "采集类型=" << displayFormat.acquisitionType
                 << "单位=" << displayFormat.unit;

        // 创建对应的通道配置
        Core::ChannelConfig channelConfig;
        channelConfig.channelId = instanceName; // 使用实例名称作为通道ID
        channelConfig.channelName = instanceName;
        channelConfig.deviceId = instanceName;
        channelConfig.hardwareChannel = "0"; // 虚拟设备只有一个通道
        channelConfig.params = channelParams;
        channelConfig.displayFormat = displayFormat;

        // 添加到通道映射
        m_channelConfigs[channelConfig.channelId] = channelConfig;

        qDebug() << "已加载虚拟设备:" << instanceName << "信号类型:" << signalType
                 << "振幅:" << amplitude << "频率:" << frequency;
    }
}

void ConfigManager::parseModbusDevices(const QJsonArray& jsonArray)
{
    // 遍历Modbus设备数组
    for (int i = 0; i < jsonArray.size(); ++i) {
        if (!jsonArray[i].isObject()) {
            qDebug() << "跳过非对象Modbus设备条目";
            continue;
        }

        QJsonObject deviceObj = jsonArray[i].toObject();

        // 提取必要字段
        QString instanceName = deviceObj["instance_name"].toString();
        int readCycleMs = deviceObj["read_cycle_ms"].toInt(1000);

        // 解析串口配置
        Core::SerialConfig serialConfig;
        if (deviceObj.contains("serial_config") && deviceObj["serial_config"].isObject()) {
            serialConfig = parseSerialConfig(deviceObj["serial_config"].toObject());
        }

        // 解析从站配置
        QList<Core::ModbusSlaveConfig> slaves;
        if (deviceObj.contains("slaves") && deviceObj["slaves"].isArray()) {
            QJsonArray slavesArray = deviceObj["slaves"].toArray();

            for (int j = 0; j < slavesArray.size(); ++j) {
                if (!slavesArray[j].isObject()) {
                    qDebug() << "跳过非对象从站条目";
                    continue;
                }

                QJsonObject slaveObj = slavesArray[j].toObject();

                // 提取从站字段
                int slaveId = slaveObj["slave_id"].toInt();
                int operationCommand = slaveObj["operation_command"].toInt(3); // 默认为3（读保持寄存器）

                // 解析寄存器配置
                QList<Core::ModbusRegisterConfig> registers;
                if (slaveObj.contains("registers") && slaveObj["registers"].isArray()) {
                    QJsonArray registersArray = slaveObj["registers"].toArray();

                    for (int k = 0; k < registersArray.size(); ++k) {
                        if (!registersArray[k].isObject()) {
                            qDebug() << "跳过非对象寄存器条目";
                            continue;
                        }

                        QJsonObject regObj = registersArray[k].toObject();

                        // 提取寄存器字段
                        int registerAddress = regObj["register_address"].toInt();
                        QString channelName = regObj["channel_name"].toString();

                        // 解析通道参数
                        Core::ChannelParams channelParams;
                        if (regObj.contains("channel_params") && regObj["channel_params"].isObject()) {
                            channelParams = parseChannelParams(regObj["channel_params"].toObject());
                        }

                        // 解析显示格式
                        Core::DisplayFormat displayFormat;
                        if (regObj.contains("display_format") && regObj["display_format"].isObject()) {
                            displayFormat = parseDisplayFormat(regObj["display_format"].toObject());
                        } else {
                            // 创建默认显示格式
                            displayFormat.labelInChinese = channelName;
                            displayFormat.acquisitionType = "modbus";
                            displayFormat.unit = channelParams.unit;
                            displayFormat.resolution = 0.01;
                            displayFormat.minRange = 0;
                            displayFormat.maxRange = 100;
                        }

                        // 创建寄存器配置
                        Core::ModbusRegisterConfig regConfig;
                        regConfig.registerAddress = registerAddress;
                        regConfig.channelName = channelName;
                        regConfig.channelParams = channelParams;
                        regConfig.displayFormat = displayFormat;

                        // 添加到寄存器列表
                        registers.append(regConfig);

                        // 添加调试输出
                        qDebug() << "Modbus寄存器显示格式:" << channelName
                                 << "中文标签=" << displayFormat.labelInChinese
                                 << "采集类型=" << displayFormat.acquisitionType
                                 << "单位=" << displayFormat.unit;

                        // 创建对应的通道配置
                        Core::ChannelConfig channelConfig;
                        channelConfig.channelId = channelName; // 直接使用通道名称作为通道ID
                        channelConfig.channelName = channelName;
                        channelConfig.deviceId = instanceName;
                        channelConfig.hardwareChannel = QString("%1_%2").arg(slaveId).arg(registerAddress);
                        channelConfig.params = channelParams;
                        channelConfig.displayFormat = displayFormat;

                        // 添加到通道映射
                        m_channelConfigs[channelConfig.channelId] = channelConfig;

                        qDebug() << "已加载Modbus寄存器:" << channelName
                                 << "从站ID:" << slaveId
                                 << "寄存器地址:" << registerAddress;
                    }
                }

                // 创建从站配置
                Core::ModbusSlaveConfig slaveConfig;
                slaveConfig.slaveId = slaveId;
                slaveConfig.operationCommand = operationCommand;
                slaveConfig.registers = registers;

                // 添加到从站列表
                slaves.append(slaveConfig);

                qDebug() << "已加载Modbus从站:" << slaveId
                         << "操作命令:" << operationCommand
                         << "寄存器数量:" << registers.size();
            }
        }

        // 创建Modbus设备配置
        Core::ModbusDeviceConfig config;
        config.deviceId = instanceName;
        config.instanceName = instanceName;
        config.serialConfig = serialConfig;
        config.readCycleMs = readCycleMs;
        config.slaves = slaves;

        // 添加到列表
        m_modbusDeviceConfigs.append(config);

        qDebug() << "已加载Modbus设备:" << instanceName
                 << "串口:" << serialConfig.port
                 << "波特率:" << serialConfig.baudrate
                 << "从站数量:" << slaves.size();
    }
}

void ConfigManager::parseDAQDevices(const QJsonArray& jsonArray)
{
    // 遍历DAQ设备数组
    for (int i = 0; i < jsonArray.size(); ++i) {
        if (!jsonArray[i].isObject()) {
            qDebug() << "跳过非对象DAQ设备条目";
            continue;
        }

        QJsonObject deviceObj = jsonArray[i].toObject();

        // 提取必要字段
        QString deviceId = deviceObj["device_id"].toString();
        int sampleRate = deviceObj["sample_rate"].toInt(10000);

        // 解析通道配置
        QList<Core::DAQChannelConfig> channels;
        if (deviceObj.contains("channels") && deviceObj["channels"].isArray()) {
            QJsonArray channelsArray = deviceObj["channels"].toArray();

            for (int j = 0; j < channelsArray.size(); ++j) {
                if (!channelsArray[j].isObject()) {
                    qDebug() << "跳过非对象DAQ通道条目";
                    continue;
                }

                QJsonObject channelObj = channelsArray[j].toObject();

                // 提取通道字段
                int channelId = channelObj["channel_id"].toInt();
                QString channelName = channelObj["channel_name"].toString();

                // 解析通道参数
                Core::ChannelParams channelParams;
                if (channelObj.contains("channel_params") && channelObj["channel_params"].isObject()) {
                    channelParams = parseChannelParams(channelObj["channel_params"].toObject());
                }

                // 解析显示格式
                Core::DisplayFormat displayFormat;
                if (channelObj.contains("display_format") && channelObj["display_format"].isObject()) {
                    displayFormat = parseDisplayFormat(channelObj["display_format"].toObject());
                } else {
                    // 创建默认显示格式
                    displayFormat.labelInChinese = channelName;
                    displayFormat.acquisitionType = "daq";
                    displayFormat.unit = channelParams.unit;
                    displayFormat.resolution = 0.01;
                    displayFormat.minRange = -10;
                    displayFormat.maxRange = 10;
                }

                // 创建通道配置
                Core::DAQChannelConfig channelConfig(channelId, channelName, channelParams);
                channelConfig.displayFormat = displayFormat;
                channels.append(channelConfig);

                // 添加调试输出
                qDebug() << "DAQ通道显示格式:" << channelName
                         << "中文标签=" << displayFormat.labelInChinese
                         << "采集类型=" << displayFormat.acquisitionType
                         << "单位=" << displayFormat.unit;

                // 创建对应的通道配置（用于数据处理）
                Core::ChannelConfig procChannelConfig;
                procChannelConfig.channelId = channelName; // 直接使用通道名称作为通道ID
                procChannelConfig.channelName = channelName;
                procChannelConfig.deviceId = deviceId;
                procChannelConfig.hardwareChannel = QString::number(channelId);
                procChannelConfig.params = channelParams;
                procChannelConfig.displayFormat = displayFormat;

                // 添加到通道映射
                m_channelConfigs[procChannelConfig.channelId] = procChannelConfig;

                qDebug() << "已加载DAQ通道:" << channelName
                         << "设备ID:" << deviceId
                         << "通道ID:" << channelId;
            }
        }

        // 创建DAQ设备配置
        Core::DAQDeviceConfig config;
        config.deviceId = deviceId;
        config.sampleRate = sampleRate;
        config.channels = channels;

        // 添加到列表
        m_daqDeviceConfigs.append(config);

        qDebug() << "已加载DAQ设备:" << deviceId
                 << "采样率:" << sampleRate
                 << "通道数量:" << channels.size();
    }
}

void ConfigManager::parseECUDevices(const QJsonArray& jsonArray)
{
    // 遍历ECU设备数组
    for (int i = 0; i < jsonArray.size(); ++i) {
        if (!jsonArray[i].isObject()) {
            qDebug() << "跳过非对象ECU设备条目";
            continue;
        }

        QJsonObject deviceObj = jsonArray[i].toObject();

        // 提取必要字段
        QString instanceName = deviceObj["instance_name"].toString();
        int readCycleMs = deviceObj["read_cycle_ms"].toInt(100);

        // 解析串口配置
        Core::SerialConfig serialConfig;
        if (deviceObj.contains("serial_config") && deviceObj["serial_config"].isObject()) {
            serialConfig = parseSerialConfig(deviceObj["serial_config"].toObject());
        }

        // 解析通道配置
        QMap<QString, Core::ECUChannelConfig> channels;

        // 检查通道配置格式
        if (deviceObj.contains("channels") && deviceObj["channels"].isArray()) {
            // 新格式：通道数组
            QJsonArray channelsArray = deviceObj["channels"].toArray();

            for (int j = 0; j < channelsArray.size(); ++j) {
                if (!channelsArray[j].isObject()) {
                    qDebug() << "跳过非对象ECU通道条目";
                    continue;
                }

                QJsonObject channelObj = channelsArray[j].toObject();

                // 提取通道字段
                QString channelName = channelObj["channel_name"].toString();
                QString hardwareChannel = channelObj["hardware_channel"].toString();

                // 解析通道参数
                Core::ChannelParams channelParams;
                if (channelObj.contains("channel_params") && channelObj["channel_params"].isObject()) {
                    channelParams = parseChannelParams(channelObj["channel_params"].toObject());
                }

                // 解析显示格式
                Core::DisplayFormat displayFormat;
                if (channelObj.contains("display_format") && channelObj["display_format"].isObject()) {
                    displayFormat = parseDisplayFormat(channelObj["display_format"].toObject());
                } else {
                    // 创建默认显示格式
                    displayFormat.labelInChinese = channelName;
                    displayFormat.acquisitionType = "ecu";
                    displayFormat.unit = channelParams.unit;
                    displayFormat.resolution = 0.01;
                    displayFormat.minRange = 0;
                    displayFormat.maxRange = 100;
                }

                // 创建ECU通道配置
                Core::ECUChannelConfig channelConfig;
                channelConfig.channelName = hardwareChannel;  // 使用硬件通道名称作为ECU内部通道名
                channelConfig.channelParams = channelParams;
                channelConfig.displayFormat = displayFormat;

                // 添加调试输出
                qDebug() << "ECU通道显示格式:" << channelName
                         << "中文标签=" << displayFormat.labelInChinese
                         << "采集类型=" << displayFormat.acquisitionType
                         << "单位=" << displayFormat.unit;

                // 添加到通道映射
                channels[hardwareChannel] = channelConfig;

                // 创建对应的通道配置（用于数据处理）
                Core::ChannelConfig procChannelConfig;
                procChannelConfig.channelId = channelName;  // 使用指定的通道名称作为通道ID
                procChannelConfig.channelName = channelName;
                procChannelConfig.deviceId = instanceName;
                procChannelConfig.hardwareChannel = hardwareChannel;
                procChannelConfig.params = channelParams;
                procChannelConfig.displayFormat = displayFormat;

                // 添加到通道映射
                m_channelConfigs[procChannelConfig.channelId] = procChannelConfig;

                qDebug() << "已加载ECU通道:" << channelName
                         << "硬件通道:" << hardwareChannel
                         << "设备:" << instanceName;
            }
        } else if (deviceObj.contains("channels") && deviceObj["channels"].isObject()) {
            // 旧格式：通道对象
            QJsonObject channelsObj = deviceObj["channels"].toObject();

            // 遍历所有通道
            for (auto it = channelsObj.begin(); it != channelsObj.end(); ++it) {
                QString channelName = it.key();

                if (!it.value().isObject()) {
                    qDebug() << "跳过非对象ECU通道条目:" << channelName;
                    continue;
                }

                QJsonObject channelObj = it.value().toObject();

                // 解析通道参数
                Core::ChannelParams channelParams;
                if (channelObj.contains("channel_params") && channelObj["channel_params"].isObject()) {
                    channelParams = parseChannelParams(channelObj["channel_params"].toObject());
                }

                // 解析显示格式
                Core::DisplayFormat displayFormat;
                if (channelObj.contains("display_format") && channelObj["display_format"].isObject()) {
                    displayFormat = parseDisplayFormat(channelObj["display_format"].toObject());
                } else {
                    // 创建默认显示格式
                    displayFormat.labelInChinese = channelName;
                    displayFormat.acquisitionType = "ecu";
                    displayFormat.unit = channelParams.unit;
                    displayFormat.resolution = 0.01;
                    displayFormat.minRange = 0;
                    displayFormat.maxRange = 100;
                }

                // 创建ECU通道配置
                Core::ECUChannelConfig channelConfig;
                channelConfig.channelName = channelName;
                channelConfig.channelParams = channelParams;
                channelConfig.displayFormat = displayFormat;

                // 添加调试输出
                qDebug() << "ECU通道显示格式(旧格式):" << channelName
                         << "中文标签=" << displayFormat.labelInChinese
                         << "采集类型=" << displayFormat.acquisitionType
                         << "单位=" << displayFormat.unit;

                // 添加到通道映射
                channels[channelName] = channelConfig;

                // 创建对应的通道配置（用于数据处理）
                Core::ChannelConfig procChannelConfig;
                procChannelConfig.channelId = channelName; // 直接使用通道名称作为通道ID
                procChannelConfig.channelName = channelName;
                procChannelConfig.deviceId = instanceName;
                procChannelConfig.hardwareChannel = channelName;
                procChannelConfig.params = channelParams;
                procChannelConfig.displayFormat = displayFormat;

                // 添加到通道映射
                m_channelConfigs[procChannelConfig.channelId] = procChannelConfig;

                qDebug() << "已加载ECU通道(旧格式):" << channelName
                         << "设备:" << instanceName;
            }
        }

        // 创建ECU设备配置
        Core::ECUDeviceConfig config;
        config.deviceId = instanceName;
        config.instanceName = instanceName;
        config.serialConfig = serialConfig;
        config.readCycleMs = readCycleMs;
        config.channels = channels;

        // 添加到列表
        m_ecuDeviceConfigs.append(config);

        qDebug() << "已加载ECU设备:" << instanceName
                 << "串口:" << serialConfig.port
                 << "波特率:" << serialConfig.baudrate
                 << "通道数量:" << channels.size();
    }
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

Core::DisplayFormat ConfigManager::parseDisplayFormat(const QJsonObject& jsonObject)
{
    Core::DisplayFormat format;

    // 提取显示格式参数
    format.labelInChinese = jsonObject["LabelInChinese"].toString("");
    format.acquisitionType = jsonObject["acquisition_type"].toString("");
    format.unit = jsonObject["unit"].toString("");
    format.resolution = jsonObject["resolution"].toDouble(0.01);
    format.minRange = jsonObject["min_range"].toDouble(0.0);
    format.maxRange = jsonObject["max_range"].toDouble(100.0);

    // 添加调试输出
    qDebug() << "解析显示格式:"
             << "中文标签=" << format.labelInChinese
             << "采集类型=" << format.acquisitionType
             << "单位=" << format.unit
             << "分辨率=" << format.resolution
             << "最小值=" << format.minRange
             << "最大值=" << format.maxRange;

    return format;
}

void ConfigManager::parseSecondaryInstruments(const QJsonArray& jsonArray)
{
    // 清空之前的配置
    m_secondaryInstrumentConfigs.clear();

    // 遍历二次计算仪器数组
    for (int i = 0; i < jsonArray.size(); ++i) {
        if (!jsonArray[i].isObject()) {
            qDebug() << "跳过非对象二次计算仪器条目";
            continue;
        }

        QJsonObject instrumentObj = jsonArray[i].toObject();

        // 提取必要字段
        QString channelName = instrumentObj["channel_name"].toString();
        QString formula = instrumentObj["formula"].toString();

        // 提取单位（如果存在）
        QString unit = instrumentObj["unit"].toString("");

        // 提取输入通道列表
        QStringList inputChannels;
        if (instrumentObj.contains("input_channels") && instrumentObj["input_channels"].isArray()) {
            QJsonArray inputChannelsArray = instrumentObj["input_channels"].toArray();
            for (int j = 0; j < inputChannelsArray.size(); ++j) {
                inputChannels.append(inputChannelsArray[j].toString());
            }
        }

        // 如果没有提供输入通道列表，尝试从公式中提取
        if (inputChannels.isEmpty()) {
            // 简单的正则表达式匹配可能的通道名称
            QRegularExpression re("[a-zA-Z_][a-zA-Z0-9_]*");
            QRegularExpressionMatchIterator i = re.globalMatch(formula);
            while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                QString potentialChannel = match.captured(0);
                // 排除常见的数学函数名
                if (potentialChannel != "sin" && potentialChannel != "cos" &&
                    potentialChannel != "tan" && potentialChannel != "sqrt" &&
                    potentialChannel != "pow" && potentialChannel != "log" &&
                    potentialChannel != "exp" && !inputChannels.contains(potentialChannel)) {
                    inputChannels.append(potentialChannel);
                }
            }
        }

        // 解析显示格式
        Core::DisplayFormat displayFormat;
        if (instrumentObj.contains("display_format") && instrumentObj["display_format"].isObject()) {
            displayFormat = parseDisplayFormat(instrumentObj["display_format"].toObject());
        } else {
            // 创建默认显示格式
            displayFormat.labelInChinese = channelName;
            displayFormat.acquisitionType = "secondary";
            displayFormat.unit = unit;
            displayFormat.resolution = 0.01;
            displayFormat.minRange = 0;
            displayFormat.maxRange = 100;
        }

        // 创建二次计算仪器配置
        Core::SecondaryInstrumentConfig config;
        config.channelName = channelName;
        config.formula = formula;
        config.inputChannels = inputChannels;
        config.unit = unit;
        config.displayFormat = displayFormat;

        // 添加到列表
        m_secondaryInstrumentConfigs.append(config);

        qDebug() << "已加载二次计算仪器:" << channelName
                 << "公式:" << formula
                 << "输入通道:" << inputChannels.join(", ")
                 << "单位:" << unit;

        // 添加显示格式调试输出
        qDebug() << "二次计算仪器显示格式:" << channelName
                 << "中文标签=" << displayFormat.labelInChinese
                 << "采集类型=" << displayFormat.acquisitionType
                 << "单位=" << displayFormat.unit
                 << "分辨率=" << displayFormat.resolution
                 << "范围=[" << displayFormat.minRange << "," << displayFormat.maxRange << "]";
    }
}

} // namespace Config
