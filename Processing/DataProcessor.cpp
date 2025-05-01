#include "DataProcessor.h"
#include <QDateTime>

namespace Processing {

DataProcessor::DataProcessor(int syncIntervalMs, QObject *parent)
    : QObject(parent)
    , m_dataSynchronizer(nullptr)
{
    qDebug() << "创建数据处理器，线程ID:" << QThread::currentThreadId();
    
    // 创建数据同步器
    m_dataSynchronizer = new DataSynchronizer(syncIntervalMs, this);
    
    // 连接数据同步器信号
    connect(m_dataSynchronizer, &DataSynchronizer::processedDataPointReady,
            this, &DataProcessor::onProcessedDataPointReady, Qt::DirectConnection);
    connect(m_dataSynchronizer, &DataSynchronizer::syncFrameReady,
            this, &DataProcessor::onSyncFrameReady, Qt::DirectConnection);
    connect(m_dataSynchronizer, &DataSynchronizer::channelStatusChanged,
            this, &DataProcessor::onChannelStatusChanged, Qt::DirectConnection);
    connect(m_dataSynchronizer, &DataSynchronizer::errorOccurred,
            this, &DataProcessor::errorOccurred, Qt::DirectConnection);
    
    qDebug() << "数据处理器初始化完成，同步间隔:" << syncIntervalMs << "毫秒";
}

DataProcessor::~DataProcessor()
{
    // 停止同步
    stopSync();
    
    // 清理通道
    QMutexLocker locker(&m_mutex);
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        delete it.value();
    }
    m_channels.clear();
    
    // 删除数据同步器
    if (m_dataSynchronizer) {
        delete m_dataSynchronizer;
        m_dataSynchronizer = nullptr;
    }
    
    qDebug() << "销毁数据处理器，线程ID:" << QThread::currentThreadId();
}

bool DataProcessor::createChannel(const Core::ChannelConfig& config)
{
    QMutexLocker locker(&m_mutex);
    
    // 检查通道是否已存在
    if (m_channels.contains(config.channelId)) {
        qDebug() << "通道已存在:" << config.channelId;
        return false;
    }
    
    // 创建通道
    Channel* channel = new Channel(config, this);
    
    // 连接通道信号
    connect(channel, &Channel::processedDataPointReady,
            this, &DataProcessor::onProcessedDataPointReady, Qt::DirectConnection);
    connect(channel, &Channel::channelStatusChanged,
            this, &DataProcessor::onChannelStatusChanged, Qt::DirectConnection);
    connect(channel, &Channel::errorOccurred,
            this, &DataProcessor::onChannelError, Qt::DirectConnection);
    
    // 添加到通道映射
    m_channels[config.channelId] = channel;
    
    qDebug() << "创建通道成功:" << config.channelId << "，线程ID:" << QThread::currentThreadId();
    return true;
}

bool DataProcessor::createChannels(const QMap<QString, Core::ChannelConfig>& configs)
{
    bool success = true;
    
    for (auto it = configs.constBegin(); it != configs.constEnd(); ++it) {
        if (!createChannel(it.value())) {
            success = false;
        }
    }
    
    return success;
}

Channel* DataProcessor::getChannel(const QString& channelId) const
{
    QMutexLocker locker(&m_mutex);
    return m_channels.value(channelId, nullptr);
}

QMap<QString, Channel*> DataProcessor::getChannels() const
{
    QMutexLocker locker(&m_mutex);
    return m_channels;
}

int DataProcessor::getSyncIntervalMs() const
{
    if (m_dataSynchronizer) {
        return m_dataSynchronizer->getSyncIntervalMs();
    }
    return Core::DEFAULT_SYNC_INTERVAL_MS;
}

void DataProcessor::setSyncIntervalMs(int intervalMs)
{
    if (m_dataSynchronizer) {
        m_dataSynchronizer->setSyncIntervalMs(intervalMs);
    }
}

void DataProcessor::startSync()
{
    if (m_dataSynchronizer) {
        m_dataSynchronizer->startSync();
        qDebug() << "数据处理器开始同步，线程ID:" << QThread::currentThreadId();
    }
}

void DataProcessor::stopSync()
{
    if (m_dataSynchronizer) {
        m_dataSynchronizer->stopSync();
        qDebug() << "数据处理器停止同步，线程ID:" << QThread::currentThreadId();
    }
}

bool DataProcessor::isSyncing() const
{
    if (m_dataSynchronizer) {
        return m_dataSynchronizer->isSyncing();
    }
    return false;
}

Core::SynchronizedDataFrame DataProcessor::getLatestSyncFrame() const
{
    if (m_dataSynchronizer) {
        return m_dataSynchronizer->getLatestSyncFrame();
    }
    return Core::SynchronizedDataFrame();
}

void DataProcessor::clearAllBuffers()
{
    if (m_dataSynchronizer) {
        m_dataSynchronizer->clearAllBuffers();
    }
}

void DataProcessor::onRawDataPointReceived(QString deviceId, QString hardwareChannel, double rawValue, qint64 timestamp)
{
    qDebug() << "数据处理器接收到原始数据点，设备ID:" << deviceId 
             << "硬件通道:" << hardwareChannel 
             << "值:" << rawValue 
             << "时间戳:" << timestamp
             << "线程ID:" << QThread::currentThreadId();
    
    // 查找对应的通道
    Channel* channel = findChannel(deviceId, hardwareChannel);
    
    if (channel) {
        // 处理原始数据点
        channel->processRawData(rawValue, timestamp);
    } else {
        qDebug() << "未找到对应的通道，设备ID:" << deviceId << "硬件通道:" << hardwareChannel;
    }
}

void DataProcessor::onDeviceStatusChanged(QString deviceId, Core::StatusCode status, QString message)
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "数据处理器接收到设备状态变化，设备ID:" << deviceId 
             << "状态:" << Core::statusCodeToString(status) 
             << "消息:" << message
             << "线程ID:" << QThread::currentThreadId();
    
    // 更新所有与该设备关联的通道状态
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        if (it.value()->getDeviceId() == deviceId) {
            it.value()->setStatus(status, message);
        }
    }
}

void DataProcessor::onChannelStatusChanged(QString channelId, Core::StatusCode status, QString message)
{
    // 转发通道状态变化信号
    emit channelStatusChanged(channelId, status, message);
    
    qDebug() << "数据处理器转发通道状态变化，通道ID:" << channelId 
             << "状态:" << Core::statusCodeToString(status) 
             << "消息:" << message
             << "线程ID:" << QThread::currentThreadId();
}

void DataProcessor::onChannelError(QString channelId, QString errorMsg)
{
    // 转发通道错误信号
    emit errorOccurred("通道 " + channelId + " 错误: " + errorMsg);
    
    qDebug() << "数据处理器转发通道错误，通道ID:" << channelId 
             << "错误:" << errorMsg
             << "线程ID:" << QThread::currentThreadId();
}

void DataProcessor::onProcessedDataPointReady(QString channelId, Core::ProcessedDataPoint dataPoint)
{
    // 转发处理后数据点就绪信号
    emit processedDataPointReady(channelId, dataPoint);
    
    qDebug() << "数据处理器转发处理后数据点，通道ID:" << channelId 
             << "值:" << dataPoint.value 
             << "时间戳:" << dataPoint.timestamp
             << "线程ID:" << QThread::currentThreadId();
}

void DataProcessor::onSyncFrameReady(Core::SynchronizedDataFrame frame)
{
    // 转发同步数据帧就绪信号
    emit syncFrameReady(frame);
    
    qDebug() << "数据处理器转发同步数据帧，时间戳:" << frame.timestamp 
             << "通道数量:" << frame.channelData.size()
             << "线程ID:" << QThread::currentThreadId();
}

Channel* DataProcessor::findChannel(const QString& deviceId, const QString& hardwareChannel) const
{
    QMutexLocker locker(&m_mutex);
    
    // 查找与设备ID和硬件通道匹配的通道
    for (auto it = m_channels.constBegin(); it != m_channels.constEnd(); ++it) {
        if (it.value()->getDeviceId() == deviceId &&
            it.value()->getHardwareChannel() == hardwareChannel) {
            return it.value();
        }
    }
    
    return nullptr;
}

} // namespace Processing
