#ifndef DAQDEVICE_H
#define DAQDEVICE_H

#include "AbstractDevice.h"
#include "../Core/DataTypes.h"
#include "../Include/Art_DAQ.h"
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QDebug>
#include <QDateTime>

// 定义错误检查宏
#define ArtDAQErrChk(functionCall) if (ArtDAQFailed(error = (functionCall))) goto Error; else

namespace Device {

// 声明回调函数
int32 ART_CALLBACK EveryNCallbackDAQ(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);
int32 ART_CALLBACK DoneCallbackDAQ(TaskHandle taskHandle, int32 status, void *callbackData);

/**
 * @brief DAQ设备类
 * 实现对DAQ设备的操作
 */
class DAQDevice : public AbstractDevice
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config DAQ设备配置
     * @param parent 父对象
     */
    explicit DAQDevice(const Core::DAQDeviceConfig& config, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~DAQDevice() override;

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

public slots:
    /**
     * @brief 设置滤波器启用状态
     * @param enabled 是否启用滤波器
     */
    void setFilterEnabled(bool enabled);

    /**
     * @brief 设置截止频率
     * @param frequency 截止频率
     */
    void setCutoffFrequency(double frequency);

    /**
     * @brief 获取滤波器启用状态
     * @return 是否启用滤波器
     */
    bool isFilterEnabled() const;

    /**
     * @brief 获取截止频率
     * @return 截止频率
     */
    double getCutoffFrequency() const;

    /**
     * @brief 初始化DAQ任务
     */
    void initializeDAQTask();

public:
    // 这些成员变量需要被回调函数访问，因此设为public
    bool isAcquiring;                    // 是否正在采集

private:
    Core::DAQDeviceConfig m_config;      // DAQ设备配置
    TaskHandle m_taskHandle;             // DAQ任务句柄
    QMutex m_mutex;                      // 互斥锁

    // 通道映射
    QMap<int, QString> m_channelNames;   // 通道ID到通道名称的映射
    QMap<int, Core::ChannelParams> m_channelParams; // 通道ID到通道参数的映射

    // 滤波器相关
    bool m_filterEnabled;                // 滤波器启用状态
    double m_cutoffFrequency;            // 截止频率
    int m_filterOrder;                   // 滤波器阶数
    QVector<QVector<double>> m_filterBuffers; // 滤波缓冲区
    QVector<double> m_filterCoefficients;     // 滤波器系数

    /**
     * @brief 处理数据
     * @param data 数据缓冲区
     * @param read 读取的样本数
     */
    void processData(float64 *data, int32 read);

    /**
     * @brief 计算滤波器系数
     */
    void calculateFilterCoefficients();

    /**
     * @brief 应用滤波器
     * @param sample 样本值
     * @param channelIndex 通道索引
     * @return 滤波后的值
     */
    double applyFilter(double sample, int channelIndex);

    /**
     * @brief 获取设备通道字符串
     * @return 设备通道字符串
     */
    QString getDeviceChannelString() const;

    // 友元声明，使回调函数可以访问私有成员
    friend int32 ART_CALLBACK EveryNCallbackDAQ(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);
    friend int32 ART_CALLBACK DoneCallbackDAQ(TaskHandle taskHandle, int32 status, void *callbackData);
};

// 全局变量，供回调函数使用
extern DAQDevice* g_daqDevice;

} // namespace Device

#endif // DAQDEVICE_H
