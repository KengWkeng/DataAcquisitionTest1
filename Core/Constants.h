#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace Core {

/**
 * @brief 设备类型枚举
 * 定义了系统支持的所有设备类型
 */
enum class DeviceType {
    VIRTUAL,    // 虚拟设备，用于测试和仿真
    MODBUS,     // Modbus设备
    DAQ,        // 数据采集卡
    ECU         // 发动机控制单元
};

/**
 * @brief 状态码枚举
 * 定义了设备和通道的各种状态
 */
enum class StatusCode {
    OK,                 // 正常
    ERROR,              // 一般错误
    ERROR_CONNECTION,   // 连接错误
    ERROR_TIMEOUT,      // 超时错误
    ERROR_DATA,         // 数据错误
    ERROR_CONFIG,       // 配置错误
    DISCONNECTED,       // 断开连接
    CONNECTING,         // 正在连接
    CONNECTED,          // 已连接
    ACQUIRING,          // 正在采集
    STOPPED             // 已停止
};

/**
 * @brief 将DeviceType转换为字符串
 * @param type 设备类型
 * @return 设备类型的字符串表示
 */
inline QString deviceTypeToString(DeviceType type) {
    switch (type) {
        case DeviceType::VIRTUAL: return "Virtual";
        case DeviceType::MODBUS: return "Modbus";
        case DeviceType::DAQ: return "DAQ";
        case DeviceType::ECU: return "ECU";
        default: return "Unknown";
    }
}

/**
 * @brief 将字符串转换为DeviceType
 * @param typeStr 设备类型的字符串表示
 * @return 设备类型枚举值
 */
inline DeviceType stringToDeviceType(const QString& typeStr) {
    if (typeStr.toLower() == "virtual") return DeviceType::VIRTUAL;
    if (typeStr.toLower() == "modbus") return DeviceType::MODBUS;
    if (typeStr.toLower() == "daq") return DeviceType::DAQ;
    if (typeStr.toLower() == "ecu") return DeviceType::ECU;
    return DeviceType::VIRTUAL; // 默认返回虚拟设备类型
}

/**
 * @brief 将StatusCode转换为字符串
 * @param status 状态码
 * @return 状态码的字符串表示
 */
inline QString statusCodeToString(StatusCode status) {
    switch (status) {
        case StatusCode::OK: return "OK";
        case StatusCode::ERROR: return "Error";
        case StatusCode::ERROR_CONNECTION: return "Connection Error";
        case StatusCode::ERROR_TIMEOUT: return "Timeout Error";
        case StatusCode::ERROR_DATA: return "Data Error";
        case StatusCode::ERROR_CONFIG: return "Configuration Error";
        case StatusCode::DISCONNECTED: return "Disconnected";
        case StatusCode::CONNECTING: return "Connecting";
        case StatusCode::CONNECTED: return "Connected";
        case StatusCode::ACQUIRING: return "Acquiring";
        case StatusCode::STOPPED: return "Stopped";
        default: return "Unknown";
    }
}

// 默认的数据同步间隔（毫秒）
constexpr int DEFAULT_SYNC_INTERVAL_MS = 100;

} // namespace Core

#endif // CONSTANTS_H
