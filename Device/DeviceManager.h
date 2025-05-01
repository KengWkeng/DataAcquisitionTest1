#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <QThread>
#include <QDebug>
#include "../Core/Constants.h"
#include "../Core/DataTypes.h"
#include "AbstractDevice.h"
#include "VirtualDevice.h"

namespace Device {

/**
 * @brief 设备管理器类
 * 负责创建、管理和控制所有设备
 */
class DeviceManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DeviceManager(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~DeviceManager();
    
    /**
     * @brief 创建设备
     * @param configs 设备配置列表
     * @return 是否成功创建所有设备
     */
    bool createDevices(const QList<Core::DeviceConfig*>& configs);
    
    /**
     * @brief 创建虚拟设备
     * @param configs 虚拟设备配置列表
     * @return 是否成功创建所有虚拟设备
     */
    bool createVirtualDevices(const QList<Core::VirtualDeviceConfig>& configs);
    
    /**
     * @brief 启动所有设备
     * @return 是否成功启动所有设备
     */
    bool startAllDevices();
    
    /**
     * @brief 停止所有设备
     * @return 是否成功停止所有设备
     */
    bool stopAllDevices();
    
    /**
     * @brief 连接所有设备
     * @return 是否成功连接所有设备
     */
    bool connectAllDevices();
    
    /**
     * @brief 断开所有设备
     * @return 是否成功断开所有设备
     */
    bool disconnectAllDevices();
    
    /**
     * @brief 获取设备
     * @param deviceId 设备ID
     * @return 设备指针，如果不存在则返回nullptr
     */
    AbstractDevice* getDevice(const QString& deviceId) const;
    
    /**
     * @brief 获取所有设备
     * @return 设备映射（设备ID -> 设备指针）
     */
    QMap<QString, AbstractDevice*> getDevices() const;
    
    /**
     * @brief 获取设备状态
     * @param deviceId 设备ID
     * @return 设备状态
     */
    Core::StatusCode getDeviceStatus(const QString& deviceId) const;

signals:
    /**
     * @brief 原始数据点就绪信号
     * @param deviceId 设备ID
     * @param hardwareChannel 硬件通道标识
     * @param rawValue 原始值
     * @param timestamp 时间戳
     */
    void rawDataPointReady(QString deviceId, QString hardwareChannel, double rawValue, qint64 timestamp);
    
    /**
     * @brief 设备状态变化信号
     * @param deviceId 设备ID
     * @param status 状态码
     * @param message 状态消息
     */
    void deviceStatusChanged(QString deviceId, Core::StatusCode status, QString message);
    
    /**
     * @brief 错误发生信号
     * @param deviceId 设备ID
     * @param errorMsg 错误消息
     */
    void errorOccurred(QString deviceId, QString errorMsg);

private:
    /**
     * @brief 创建设备线程
     * @param device 设备指针
     * @return 是否成功创建线程
     */
    bool createDeviceThread(AbstractDevice* device);
    
    /**
     * @brief 清理设备和线程
     */
    void cleanup();

private:
    QMap<QString, AbstractDevice*> m_devices;    // 设备映射（设备ID -> 设备指针）
    QMap<QString, QThread*> m_deviceThreads;     // 设备线程映射（设备ID -> 线程指针）
};

} // namespace Device

#endif // DEVICEMANAGER_H
