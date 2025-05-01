#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QThread>
#include <QDebug>
#include "DataSynchronizer.h"
#include "Channel.h"
#include "../Core/DataTypes.h"

namespace Processing {

/**
 * @brief 数据处理器类
 * 统一管理数据同步和通道计算，在单独的线程中运行
 */
class DataProcessor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param syncIntervalMs 同步间隔（毫秒）
     * @param parent 父对象
     */
    explicit DataProcessor(int syncIntervalMs = Core::DEFAULT_SYNC_INTERVAL_MS, QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~DataProcessor();
    
    /**
     * @brief 创建通道
     * @param config 通道配置
     * @return 是否成功创建
     */
    bool createChannel(const Core::ChannelConfig& config);
    
    /**
     * @brief 创建多个通道
     * @param configs 通道配置映射
     * @return 是否成功创建所有通道
     */
    bool createChannels(const QMap<QString, Core::ChannelConfig>& configs);
    
    /**
     * @brief 获取通道
     * @param channelId 通道ID
     * @return 通道指针，如果不存在则返回nullptr
     */
    Channel* getChannel(const QString& channelId) const;
    
    /**
     * @brief 获取所有通道
     * @return 通道映射（通道ID -> 通道指针）
     */
    QMap<QString, Channel*> getChannels() const;
    
    /**
     * @brief 获取同步间隔（毫秒）
     * @return 同步间隔
     */
    int getSyncIntervalMs() const;
    
    /**
     * @brief 设置同步间隔（毫秒）
     * @param intervalMs 同步间隔
     */
    void setSyncIntervalMs(int intervalMs);
    
    /**
     * @brief 开始同步
     */
    void startSync();
    
    /**
     * @brief 停止同步
     */
    void stopSync();
    
    /**
     * @brief 是否正在同步
     * @return 是否正在同步
     */
    bool isSyncing() const;
    
    /**
     * @brief 获取最新的同步数据帧
     * @return 同步数据帧
     */
    Core::SynchronizedDataFrame getLatestSyncFrame() const;
    
    /**
     * @brief 清除所有通道的数据缓冲区
     */
    void clearAllBuffers();

public slots:
    /**
     * @brief 处理原始数据点
     * @param deviceId 设备ID
     * @param hardwareChannel 硬件通道标识
     * @param rawValue 原始值
     * @param timestamp 时间戳
     */
    void onRawDataPointReceived(QString deviceId, QString hardwareChannel, double rawValue, qint64 timestamp);
    
    /**
     * @brief 处理设备状态变化
     * @param deviceId 设备ID
     * @param status 状态码
     * @param message 状态消息
     */
    void onDeviceStatusChanged(QString deviceId, Core::StatusCode status, QString message);

signals:
    /**
     * @brief 处理后数据点就绪信号
     * @param channelId 通道ID
     * @param dataPoint 处理后的数据点
     */
    void processedDataPointReady(QString channelId, Core::ProcessedDataPoint dataPoint);
    
    /**
     * @brief 同步数据帧就绪信号
     * @param frame 同步数据帧
     */
    void syncFrameReady(Core::SynchronizedDataFrame frame);
    
    /**
     * @brief 通道状态变化信号
     * @param channelId 通道ID
     * @param status 状态码
     * @param message 状态消息
     */
    void channelStatusChanged(QString channelId, Core::StatusCode status, QString message);
    
    /**
     * @brief 错误发生信号
     * @param errorMsg 错误消息
     */
    void errorOccurred(QString errorMsg);

private slots:
    /**
     * @brief 处理通道状态变化
     * @param channelId 通道ID
     * @param status 状态码
     * @param message 状态消息
     */
    void onChannelStatusChanged(QString channelId, Core::StatusCode status, QString message);
    
    /**
     * @brief 处理通道错误
     * @param channelId 通道ID
     * @param errorMsg 错误消息
     */
    void onChannelError(QString channelId, QString errorMsg);
    
    /**
     * @brief 处理处理后数据点
     * @param channelId 通道ID
     * @param dataPoint 处理后的数据点
     */
    void onProcessedDataPointReady(QString channelId, Core::ProcessedDataPoint dataPoint);
    
    /**
     * @brief 处理同步数据帧就绪
     * @param frame 同步数据帧
     */
    void onSyncFrameReady(Core::SynchronizedDataFrame frame);

private:
    /**
     * @brief 查找通道
     * 根据设备ID和硬件通道标识查找对应的通道
     * @param deviceId 设备ID
     * @param hardwareChannel 硬件通道标识
     * @return 通道指针，如果不存在则返回nullptr
     */
    Channel* findChannel(const QString& deviceId, const QString& hardwareChannel) const;

private:
    DataSynchronizer* m_dataSynchronizer;        // 数据同步器
    QMap<QString, Channel*> m_channels;          // 通道映射（通道ID -> 通道指针）
    mutable QMutex m_mutex;                      // 互斥锁
};

} // namespace Processing

#endif // DATAPROCESSOR_H
