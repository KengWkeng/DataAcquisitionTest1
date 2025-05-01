#ifndef ABSTRACTDEVICE_H
#define ABSTRACTDEVICE_H

#include <QObject>
#include <QString>
#include <QDebug>
#include "../Core/Constants.h"
#include "../Core/DataTypes.h"

namespace Device {

/**
 * @brief 设备抽象基类
 * 所有设备类型的基类，定义了通用接口
 */
class AbstractDevice : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit AbstractDevice(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    virtual ~AbstractDevice();
    
    /**
     * @brief 连接设备
     * @return 是否成功连接
     */
    virtual bool connectDevice() = 0;
    
    /**
     * @brief 断开设备连接
     * @return 是否成功断开
     */
    virtual bool disconnectDevice() = 0;
    
    /**
     * @brief 开始数据采集
     */
    virtual void startAcquisition() = 0;
    
    /**
     * @brief 停止数据采集
     */
    virtual void stopAcquisition() = 0;
    
    /**
     * @brief 获取设备ID
     * @return 设备ID
     */
    virtual QString getDeviceId() const = 0;
    
    /**
     * @brief 获取设备类型
     * @return 设备类型
     */
    virtual Core::DeviceType getDeviceType() const = 0;
    
    /**
     * @brief 获取设备状态
     * @return 设备状态
     */
    virtual Core::StatusCode getStatus() const;
    
    /**
     * @brief 应用滤波器
     * 子类可以重写此方法实现特定的滤波算法
     * @param rawValue 原始值
     * @return 滤波后的值
     */
    virtual double applyFilter(double rawValue);

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

protected:
    /**
     * @brief 设置设备状态
     * @param status 状态码
     * @param message 状态消息
     */
    void setStatus(Core::StatusCode status, const QString& message = QString());

protected:
    Core::StatusCode m_status; // 设备状态
    QString m_statusMessage;   // 状态消息
};

} // namespace Device

#endif // ABSTRACTDEVICE_H
