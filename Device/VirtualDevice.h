#ifndef VIRTUALDEVICE_H
#define VIRTUALDEVICE_H

#include "AbstractDevice.h"
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QtMath>

namespace Device {

/**
 * @brief 虚拟设备类
 * 用于测试和仿真的虚拟设备
 */
class VirtualDevice : public AbstractDevice
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config 虚拟设备配置
     * @param parent 父对象
     */
    explicit VirtualDevice(const Core::VirtualDeviceConfig& config, QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~VirtualDevice() override;
    
    /**
     * @brief 连接设备
     * @return 是否成功连接
     */
    bool connectDevice() override;
    
    /**
     * @brief 断开设备连接
     * @return 是否成功断开
     */
    bool disconnectDevice() override;
    
    /**
     * @brief 开始数据采集
     */
    void startAcquisition() override;
    
    /**
     * @brief 停止数据采集
     */
    void stopAcquisition() override;
    
    /**
     * @brief 获取设备ID
     * @return 设备ID
     */
    QString getDeviceId() const override;
    
    /**
     * @brief 获取设备类型
     * @return 设备类型
     */
    Core::DeviceType getDeviceType() const override;

private slots:
    /**
     * @brief 生成数据点
     * 定时器触发时生成一个新的数据点
     */
    void generateDataPoint();

private:
    /**
     * @brief 生成正弦波数据
     * @return 正弦波数据值
     */
    double generateSineWave();
    
    /**
     * @brief 生成方波数据
     * @return 方波数据值
     */
    double generateSquareWave();
    
    /**
     * @brief 生成三角波数据
     * @return 三角波数据值
     */
    double generateTriangleWave();
    
    /**
     * @brief 生成随机数据
     * @return 随机数据值
     */
    double generateRandomValue();

private:
    Core::VirtualDeviceConfig m_config;  // 设备配置
    QTimer* m_timer;                     // 定时器
    qint64 m_startTime;                  // 开始时间
    double m_phase;                      // 相位（用于波形生成）
};

} // namespace Device

#endif // VIRTUALDEVICE_H
