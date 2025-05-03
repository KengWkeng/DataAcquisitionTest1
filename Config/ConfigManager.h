#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

#include "../Core/Constants.h"
#include "../Core/DataTypes.h"

namespace Config {

/**
 * @brief 配置管理器类
 * 负责读取、解析和提供配置信息
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit ConfigManager(QObject *parent = nullptr);

    /**
     * @brief 加载配置文件
     * @param filePath 配置文件路径
     * @return 是否成功加载
     */
    bool loadConfig(const QString& filePath);

    /**
     * @brief 获取所有设备配置
     * @return 设备配置列表
     */
    QList<Core::DeviceConfig*> getDeviceConfigs() const;

    /**
     * @brief 获取虚拟设备配置
     * @return 虚拟设备配置列表
     */
    QList<Core::VirtualDeviceConfig> getVirtualDeviceConfigs() const;

    /**
     * @brief 获取Modbus设备配置
     * @return Modbus设备配置列表
     */
    QList<Core::ModbusDeviceConfig> getModbusDeviceConfigs() const;

    /**
     * @brief 获取DAQ设备配置
     * @return DAQ设备配置列表
     */
    QList<Core::DAQDeviceConfig> getDAQDeviceConfigs() const;

    /**
     * @brief 获取ECU设备配置
     * @return ECU设备配置列表
     */
    QList<Core::ECUDeviceConfig> getECUDeviceConfigs() const;

    /**
     * @brief 获取通道配置
     * @return 通道配置映射（通道ID -> 通道配置）
     */
    QMap<QString, Core::ChannelConfig> getChannelConfigs() const;

    /**
     * @brief 获取二次计算仪器配置
     * @return 二次计算仪器配置列表
     */
    QList<Core::SecondaryInstrumentConfig> getSecondaryInstrumentConfigs() const;

    /**
     * @brief 获取数据同步间隔（毫秒）
     * @return 同步间隔
     */
    int getSynchronizationIntervalMs() const;

    /**
     * @brief 获取配置文件路径
     * @return 配置文件路径
     */
    QString getConfigFilePath() const;

    /**
     * @brief 保存配置到文件
     * @param filePath 文件路径
     * @return 是否成功保存
     */
    bool saveConfig(const QString& filePath) const;

private:
    /**
     * @brief 解析虚拟设备配置
     * @param jsonArray 虚拟设备JSON数组
     */
    void parseVirtualDevices(const QJsonArray& jsonArray);

    /**
     * @brief 解析Modbus设备配置
     * @param jsonArray Modbus设备JSON数组
     */
    void parseModbusDevices(const QJsonArray& jsonArray);

    /**
     * @brief 解析DAQ设备配置
     * @param jsonArray DAQ设备JSON数组
     */
    void parseDAQDevices(const QJsonArray& jsonArray);

    /**
     * @brief 解析ECU设备配置
     * @param jsonArray ECU设备JSON数组
     */
    void parseECUDevices(const QJsonArray& jsonArray);

    /**
     * @brief 解析二次计算仪器配置
     * @param jsonArray 二次计算仪器JSON数组
     */
    void parseSecondaryInstruments(const QJsonArray& jsonArray);

    /**
     * @brief 从JSON对象解析通道参数
     * @param jsonObject 包含通道参数的JSON对象
     * @return 通道参数
     */
    Core::ChannelParams parseChannelParams(const QJsonObject& jsonObject);

    /**
     * @brief 从JSON对象解析校准参数
     * @param jsonObject 包含校准参数的JSON对象
     * @return 校准参数
     */
    Core::CalibrationParams parseCalibrationParams(const QJsonObject& jsonObject);

    /**
     * @brief 从JSON对象解析串口配置
     * @param jsonObject 包含串口配置的JSON对象
     * @return 串口配置
     */
    Core::SerialConfig parseSerialConfig(const QJsonObject& jsonObject);

    /**
     * @brief 从JSON对象解析显示格式
     * @param jsonObject 包含显示格式的JSON对象
     * @return 显示格式
     */
    Core::DisplayFormat parseDisplayFormat(const QJsonObject& jsonObject);

private:
    QString m_configFilePath;                                // 配置文件路径
    QList<Core::VirtualDeviceConfig> m_virtualDeviceConfigs; // 虚拟设备配置列表
    QList<Core::ModbusDeviceConfig> m_modbusDeviceConfigs;   // Modbus设备配置列表
    QList<Core::DAQDeviceConfig> m_daqDeviceConfigs;         // DAQ设备配置列表
    QList<Core::ECUDeviceConfig> m_ecuDeviceConfigs;         // ECU设备配置列表
    QList<Core::SecondaryInstrumentConfig> m_secondaryInstrumentConfigs; // 二次计算仪器配置列表
    QMap<QString, Core::ChannelConfig> m_channelConfigs;     // 通道配置映射
    int m_synchronizationIntervalMs;                         // 数据同步间隔（毫秒）
};

} // namespace Config

#endif // CONFIGMANAGER_H
