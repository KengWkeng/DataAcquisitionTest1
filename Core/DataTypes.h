#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QMap>
#include <QList>
#include <QVariant>
#include <QDateTime>
#include "Constants.h"

namespace Core {

/**
 * @brief 立方校准参数结构体
 * 用于存储校准多项式的系数: ax^3 + bx^2 + cx + d
 */
struct CalibrationParams {
    double a = 0.0;  // 三次项系数
    double b = 0.0;  // 二次项系数
    double c = 1.0;  // 一次项系数
    double d = 0.0;  // 常数项

    CalibrationParams() = default;

    CalibrationParams(double a_, double b_, double c_, double d_)
        : a(a_), b(b_), c(c_), d(d_) {}

    /**
     * @brief 应用校准多项式计算校准后的值
     * @param value 输入值
     * @return 校准后的值
     */
    double apply(double value) const {
        return a * value * value * value + b * value * value + c * value + d;
    }
};

/**
 * @brief 显示格式结构体
 * 存储通道的显示相关参数
 */
struct DisplayFormat {
    QString labelInChinese;     // 中文标签
    QString acquisitionType;    // 采集类型
    QString unit;               // 单位
    double resolution = 0.01;   // 分辨率
    double minRange = 0.0;      // 最小范围值
    double maxRange = 100.0;    // 最大范围值

    DisplayFormat() = default;

    DisplayFormat(const QString& label, const QString& type, const QString& u,
                 double res, double min, double max)
        : labelInChinese(label), acquisitionType(type), unit(u),
          resolution(res), minRange(min), maxRange(max) {}
};

/**
 * @brief 通道参数结构体
 * 存储通道的增益、偏移和校准参数
 */
struct ChannelParams {
    double gain = 1.0;                  // 增益
    double offset = 0.0;                // 偏移
    CalibrationParams calibrationParams; // 校准参数
    QString unit;                       // 单位

    ChannelParams() = default;

    ChannelParams(double gain_, double offset_,
                 const CalibrationParams& calibParams_,
                 const QString& unit_ = "")
        : gain(gain_), offset(offset_), calibrationParams(calibParams_), unit(unit_) {}

    /**
     * @brief 应用增益和偏移
     * @param rawValue 原始值
     * @return 应用增益和偏移后的值
     */
    double applyGainOffset(double rawValue) const {
        return rawValue * gain + offset;
    }

    /**
     * @brief 应用完整的处理（增益、偏移和校准）
     * @param rawValue 原始值
     * @return 完全处理后的值
     */
    double process(double rawValue) const {
        double valueWithGainOffset = applyGainOffset(rawValue);
        return calibrationParams.apply(valueWithGainOffset);
    }
};

/**
 * @brief 设备配置基类
 * 所有设备配置的基类
 */
struct DeviceConfig {
    QString deviceId;       // 设备ID
    DeviceType deviceType;  // 设备类型

    DeviceConfig() = default;

    DeviceConfig(const QString& id, DeviceType type)
        : deviceId(id), deviceType(type) {}

    virtual ~DeviceConfig() = default;
};

/**
 * @brief 虚拟设备配置
 * 用于配置虚拟设备的参数
 */
struct VirtualDeviceConfig : public DeviceConfig {
    QString instanceName;   // 实例名称
    QString signalType;     // 信号类型：sine, square, triangle, random
    double amplitude;       // 振幅
    double frequency;       // 频率 (Hz)
    ChannelParams channelParams; // 通道参数
    DisplayFormat displayFormat; // 显示格式

    VirtualDeviceConfig() {
        deviceType = DeviceType::VIRTUAL;
    }

    VirtualDeviceConfig(const QString& id, const QString& name,
                       const QString& type, double amp, double freq,
                       const ChannelParams& params)
        : DeviceConfig(id, DeviceType::VIRTUAL),
          instanceName(name), signalType(type),
          amplitude(amp), frequency(freq), channelParams(params) {}

    VirtualDeviceConfig(const QString& id, const QString& name,
                       const QString& type, double amp, double freq,
                       const ChannelParams& params, const DisplayFormat& df)
        : DeviceConfig(id, DeviceType::VIRTUAL),
          instanceName(name), signalType(type),
          amplitude(amp), frequency(freq), channelParams(params), displayFormat(df) {}
};

/**
 * @brief Modbus寄存器配置
 * 配置Modbus设备的寄存器
 */
struct ModbusRegisterConfig {
    int registerAddress;     // 寄存器地址
    QString channelName;     // 通道名称
    ChannelParams channelParams; // 通道参数
    DisplayFormat displayFormat; // 显示格式

    ModbusRegisterConfig() = default;

    ModbusRegisterConfig(int addr, const QString& name, const ChannelParams& params)
        : registerAddress(addr), channelName(name), channelParams(params) {}

    ModbusRegisterConfig(int addr, const QString& name, const ChannelParams& params, const DisplayFormat& df)
        : registerAddress(addr), channelName(name), channelParams(params), displayFormat(df) {}
};

/**
 * @brief Modbus从站配置
 * 配置Modbus设备的从站
 */
struct ModbusSlaveConfig {
    int slaveId;                        // 从站ID
    int operationCommand;               // 操作命令（功能码）
    QList<ModbusRegisterConfig> registers; // 寄存器列表

    ModbusSlaveConfig() = default;

    ModbusSlaveConfig(int id, int cmd, const QList<ModbusRegisterConfig>& regs)
        : slaveId(id), operationCommand(cmd), registers(regs) {}
};

/**
 * @brief 串口配置
 * 配置串口参数
 */
struct SerialConfig {
    QString port;       // 端口名
    int baudrate;       // 波特率
    int databits;       // 数据位
    int stopbits;       // 停止位
    QString parity;     // 校验位

    SerialConfig() = default;

    SerialConfig(const QString& p, int baud, int data, int stop, const QString& par)
        : port(p), baudrate(baud), databits(data), stopbits(stop), parity(par) {}
};

/**
 * @brief Modbus设备配置
 * 用于配置Modbus设备
 */
struct ModbusDeviceConfig : public DeviceConfig {
    QString instanceName;                // 实例名称
    SerialConfig serialConfig;           // 串口配置
    int readCycleMs;                     // 读取周期（毫秒）
    QList<ModbusSlaveConfig> slaves;     // 从站列表

    ModbusDeviceConfig() {
        deviceType = DeviceType::MODBUS;
    }

    ModbusDeviceConfig(const QString& id, const QString& name,
                      const SerialConfig& serial, int cycle,
                      const QList<ModbusSlaveConfig>& slaveList)
        : DeviceConfig(id, DeviceType::MODBUS),
          instanceName(name), serialConfig(serial),
          readCycleMs(cycle), slaves(slaveList) {}
};

/**
 * @brief DAQ通道配置
 * 配置DAQ设备的通道
 */
struct DAQChannelConfig {
    int channelId;           // 通道ID
    QString channelName;     // 通道名称
    ChannelParams channelParams; // 通道参数
    DisplayFormat displayFormat; // 显示格式

    DAQChannelConfig() = default;

    DAQChannelConfig(int id, const QString& name, const ChannelParams& params)
        : channelId(id), channelName(name), channelParams(params) {}

    DAQChannelConfig(int id, const QString& name, const ChannelParams& params, const DisplayFormat& df)
        : channelId(id), channelName(name), channelParams(params), displayFormat(df) {}
};

/**
 * @brief DAQ设备配置
 * 用于配置DAQ设备
 */
struct DAQDeviceConfig : public DeviceConfig {
    int sampleRate;                      // 采样率
    QList<DAQChannelConfig> channels;    // 通道列表

    DAQDeviceConfig() {
        deviceType = DeviceType::DAQ;
    }

    DAQDeviceConfig(const QString& id, int rate, const QList<DAQChannelConfig>& chans)
        : DeviceConfig(id, DeviceType::DAQ),
          sampleRate(rate), channels(chans) {}
};

/**
 * @brief ECU通道配置
 * 配置ECU设备的通道
 */
struct ECUChannelConfig {
    QString channelName;     // 通道名称
    ChannelParams channelParams; // 通道参数
    DisplayFormat displayFormat; // 显示格式

    ECUChannelConfig() = default;

    ECUChannelConfig(const QString& name, const ChannelParams& params)
        : channelName(name), channelParams(params) {}

    ECUChannelConfig(const QString& name, const ChannelParams& params, const DisplayFormat& df)
        : channelName(name), channelParams(params), displayFormat(df) {}
};

/**
 * @brief ECU设备配置
 * 用于配置ECU设备
 */
struct ECUDeviceConfig : public DeviceConfig {
    QString instanceName;                        // 实例名称
    SerialConfig serialConfig;                   // 串口配置
    int readCycleMs;                             // 读取周期（毫秒）
    QMap<QString, ECUChannelConfig> channels;    // 通道映射

    ECUDeviceConfig() {
        deviceType = DeviceType::ECU;
    }

    ECUDeviceConfig(const QString& id, const QString& name,
                   const SerialConfig& serial, int cycle,
                   const QMap<QString, ECUChannelConfig>& chans)
        : DeviceConfig(id, DeviceType::ECU),
          instanceName(name), serialConfig(serial),
          readCycleMs(cycle), channels(chans) {}
};

/**
 * @brief 通道配置
 * 软件通道的配置
 */
struct ChannelConfig {
    QString channelId;       // 通道ID
    QString channelName;     // 通道名称
    QString deviceId;        // 关联的设备ID
    QString hardwareChannel; // 硬件通道标识（可能是索引或名称）
    ChannelParams params;    // 通道参数
    DisplayFormat displayFormat; // 显示格式

    ChannelConfig() = default;

    ChannelConfig(const QString& id, const QString& name,
                 const QString& devId, const QString& hwChan,
                 const ChannelParams& p)
        : channelId(id), channelName(name),
          deviceId(devId), hardwareChannel(hwChan),
          params(p) {}

    ChannelConfig(const QString& id, const QString& name,
                 const QString& devId, const QString& hwChan,
                 const ChannelParams& p, const DisplayFormat& df)
        : channelId(id), channelName(name),
          deviceId(devId), hardwareChannel(hwChan),
          params(p), displayFormat(df) {}
};

/**
 * @brief 原始数据点
 * 从设备读取的原始数据
 */
struct RawDataPoint {
    double value;            // 原始值
    qint64 timestamp;        // 时间戳（毫秒）
    QString deviceId;        // 设备ID
    QString hardwareChannel; // 硬件通道标识

    RawDataPoint() = default;

    RawDataPoint(double val, qint64 ts, const QString& devId, const QString& hwChan)
        : value(val), timestamp(ts), deviceId(devId), hardwareChannel(hwChan) {}
};

/**
 * @brief 处理后的数据点
 * 经过处理的数据点
 */
struct ProcessedDataPoint {
    double value;            // 处理后的值
    qint64 timestamp;        // 时间戳（毫秒）
    QString channelId;       // 通道ID
    StatusCode status;       // 状态码
    QString unit;            // 单位

    ProcessedDataPoint() : status(StatusCode::OK) {}

    ProcessedDataPoint(double val, qint64 ts, const QString& chanId,
                      StatusCode stat = StatusCode::OK, const QString& u = "")
        : value(val), timestamp(ts), channelId(chanId), status(stat), unit(u) {}
};

/**
 * @brief 二次计算仪器配置
 * 用于配置二次计算仪器
 */
struct SecondaryInstrumentConfig {
    QString channelName;         // 通道名称
    QString formula;             // 计算公式
    QStringList inputChannels;   // 输入通道列表
    QString unit;                // 单位
    DisplayFormat displayFormat; // 显示格式

    SecondaryInstrumentConfig() = default;

    SecondaryInstrumentConfig(const QString& name, const QString& form,
                             const QStringList& inputs, const QString& u = "")
        : channelName(name), formula(form), inputChannels(inputs), unit(u) {}

    SecondaryInstrumentConfig(const QString& name, const QString& form,
                             const QStringList& inputs, const QString& u,
                             const DisplayFormat& df)
        : channelName(name), formula(form), inputChannels(inputs), unit(u),
          displayFormat(df) {}
};

/**
 * @brief 同步数据帧
 * 包含特定时间点的所有通道数据
 */
struct SynchronizedDataFrame {
    qint64 timestamp;                                    // 时间戳（毫秒）
    QMap<QString, ProcessedDataPoint> channelData;       // 通道数据映射

    SynchronizedDataFrame() = default;

    SynchronizedDataFrame(qint64 ts)
        : timestamp(ts) {}

    /**
     * @brief 添加通道数据
     * @param channelId 通道ID
     * @param dataPoint 处理后的数据点
     */
    void addChannelData(const QString& channelId, const ProcessedDataPoint& dataPoint) {
        channelData[channelId] = dataPoint;
    }

    /**
     * @brief 获取通道数据
     * @param channelId 通道ID
     * @return 处理后的数据点
     */
    ProcessedDataPoint getChannelData(const QString& channelId) const {
        return channelData.value(channelId);
    }

    /**
     * @brief 获取通道值
     * @param channelId 通道ID
     * @param defaultValue 默认值
     * @return 通道值，如果通道不存在则返回默认值
     */
    double getChannelValue(const QString& channelId, double defaultValue = 0.0) const {
        auto it = channelData.find(channelId);
        if (it != channelData.end()) {
            return it.value().value;
        }
        return defaultValue;
    }

    /**
     * @brief 获取格式化的时间戳
     * @param format 格式字符串
     * @return 格式化的时间戳
     */
    QString getFormattedTimestamp(const QString& format = "yyyy-MM-dd hh:mm:ss.zzz") const {
        return QDateTime::fromMSecsSinceEpoch(timestamp).toString(format);
    }
};

} // namespace Core

#endif // DATATYPES_H
