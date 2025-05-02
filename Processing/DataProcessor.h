#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QQueue>
#include <QVector>
#include <QReadWriteLock>
#include "../Core/Constants.h"
#include "../Core/DataTypes.h"
#include "Channel.h"

namespace Processing {

/**
 * @brief 数据处理器类
 * 负责数据同步和通道处理
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
     * @brief 开始处理
     */
    void startProcessing();

    /**
     * @brief 停止处理
     */
    void stopProcessing();

    /**
     * @brief 是否正在处理
     * @return 是否正在处理
     */
    bool isProcessing() const;

    /**
     * @brief 获取最新的同步数据帧
     * @return 同步数据帧
     */
    Core::SynchronizedDataFrame getLatestSyncFrame() const;

    /**
     * @brief 获取通道的处理后数据
     * @param channelId 通道ID
     * @param timestamps 时间戳向量（输出）
     * @param values 值向量（输出）
     * @param maxPoints 最大点数
     * @return 是否成功获取数据
     */
    bool getChannelData(const QString& channelId, QVector<double>& timestamps, QVector<double>& values, int maxPoints = -1) const;

    /**
     * @brief 获取所有通道的最新数据点
     * @return 通道ID到最新数据点的映射
     */
    QMap<QString, QPair<double, double>> getLatestDataPoints() const;

    /**
     * @brief 清除所有缓冲区
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
     * @brief 执行同步和处理
     * 定时器触发时同步和处理数据
     */
    void performProcessing();

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

private:
    /**
     * @brief 查找通道
     * 根据设备ID和硬件通道标识查找对应的通道
     * @param deviceId 设备ID
     * @param hardwareChannel 硬件通道标识
     * @return 通道指针，如果不存在则返回nullptr
     */
    Channel* findChannel(const QString& deviceId, const QString& hardwareChannel) const;

    /**
     * @brief 处理原始数据
     * 对原始数据进行处理并生成同步数据帧
     * @return 同步数据帧
     */
    Core::SynchronizedDataFrame processData();

private:
    // 通道管理
    QMap<QString, Channel*> m_channels;                  // 通道映射（通道ID -> 通道指针）

    // 原始数据缓存
    struct RawDataPoint {
        double value;
        qint64 timestamp;
    };
    QMap<QPair<QString, QString>, RawDataPoint> m_rawDataCache; // 原始数据缓存 (设备ID,硬件通道) -> 原始数据点

    // 处理后数据缓存
    struct ProcessedDataQueue {
        QQueue<qint64> timestamps;                       // 时间戳队列
        QQueue<double> values;                           // 值队列
        int maxSize;                                     // 最大队列长度
    };
    QMap<QString, ProcessedDataQueue> m_processedDataQueues; // 处理后数据队列 (通道ID -> 数据队列)

    // 同步和处理
    QTimer* m_processingTimer;                           // 处理定时器
    int m_syncIntervalMs;                                // 同步间隔（毫秒）
    Core::SynchronizedDataFrame m_latestSyncFrame;       // 最新的同步数据帧

    // 线程安全
    mutable QMutex m_mutex;                              // 互斥锁
    mutable QReadWriteLock m_dataLock;                   // 数据读写锁
    bool m_isProcessing;                                 // 是否正在处理

    // 常量
    static const int MAX_QUEUE_SIZE = 1000;              // 最大队列长度
};

} // namespace Processing

#endif // DATAPROCESSOR_H
