#ifndef CHANNEL_H
#define CHANNEL_H

#include <QObject>
#include <QString>
#include <QQueue>
#include <QMutex>
#include <QDebug>
#include "../Core/Constants.h"
#include "../Core/DataTypes.h"

namespace Processing {

/**
 * @brief 通道类
 * 负责处理和存储单个通道的数据
 */
class Channel : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config 通道配置
     * @param parent 父对象
     */
    explicit Channel(const Core::ChannelConfig& config, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~Channel();

    /**
     * @brief 处理原始数据点
     * @param rawValue 原始值
     * @param timestamp 时间戳
     * @return 处理后的数据点
     */
    Core::ProcessedDataPoint processRawData(double rawValue, qint64 timestamp);

    /**
     * @brief 获取最新的处理后数据点
     * @return 最新的处理后数据点
     */
    Core::ProcessedDataPoint getLatestProcessedDataPoint() const;

    /**
     * @brief 获取通道ID
     * @return 通道ID
     */
    QString getChannelId() const;

    /**
     * @brief 获取通道名称
     * @return 通道名称
     */
    QString getChannelName() const;

    /**
     * @brief 获取设备ID
     * @return 设备ID
     */
    QString getDeviceId() const;

    /**
     * @brief 获取硬件通道标识
     * @return 硬件通道标识
     */
    QString getHardwareChannel() const;

    /**
     * @brief 获取通道参数
     * @return 通道参数
     */
    Core::ChannelParams getParams() const;

    /**
     * @brief 设置通道参数
     * @param params 通道参数
     */
    void setParams(const Core::ChannelParams& params);

    /**
     * @brief 获取通道状态
     * @return 通道状态
     */
    Core::StatusCode getStatus() const;

    /**
     * @brief 设置通道状态
     * @param status 通道状态
     * @param message 状态消息
     */
    void setStatus(Core::StatusCode status, const QString& message = QString());

signals:
    /**
     * @brief 通道状态变化信号
     * @param channelId 通道ID
     * @param status 状态码
     * @param message 状态消息
     */
    void channelStatusChanged(QString channelId, Core::StatusCode status, QString message);

    /**
     * @brief 错误发生信号
     * @param channelId 通道ID
     * @param errorMsg 错误消息
     */
    void errorOccurred(QString channelId, QString errorMsg);

private:
    /**
     * @brief 应用增益和偏移
     * @param rawValue 原始值
     * @return 应用增益和偏移后的值
     */
    double applyGainOffset(double rawValue) const;

    /**
     * @brief 应用校准
     * @param value 输入值
     * @return 校准后的值
     */
    double applyCalibration(double value) const;

private:
    Core::ChannelConfig m_config;                // 通道配置
    Core::ProcessedDataPoint m_latestDataPoint;  // 最新的处理后数据点
    mutable QMutex m_mutex;                      // 互斥锁
    Core::StatusCode m_status;                   // 通道状态
    QString m_statusMessage;                     // 状态消息
};

} // namespace Processing

#endif // CHANNEL_H
