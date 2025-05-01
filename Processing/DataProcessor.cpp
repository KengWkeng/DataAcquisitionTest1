#include "DataProcessor.h"
#include <QDateTime>

namespace Processing {

DataProcessor::DataProcessor(int syncIntervalMs, QObject *parent)
    : QObject(parent)
    , m_processingTimer(new QTimer(this))
    , m_syncIntervalMs(syncIntervalMs)
    , m_isProcessing(false)
{
    // 连接处理定时器信号
    connect(m_processingTimer, &QTimer::timeout, this, &DataProcessor::performProcessing);

    // 设置处理定时器间隔
    m_processingTimer->setInterval(m_syncIntervalMs);

    qDebug() << "创建数据处理器，同步间隔:" << m_syncIntervalMs << "毫秒，线程ID:" << QThread::currentThreadId();
}

DataProcessor::~DataProcessor()
{
    // 停止处理
    stopProcessing();

    // 清理通道
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        delete it.value();
    }
    m_channels.clear();

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
    return m_syncIntervalMs;
}

void DataProcessor::setSyncIntervalMs(int intervalMs)
{
    if (intervalMs > 0 && intervalMs != m_syncIntervalMs) {
        m_syncIntervalMs = intervalMs;
        m_processingTimer->setInterval(m_syncIntervalMs);
        qDebug() << "设置同步间隔为:" << m_syncIntervalMs << "毫秒";
    }
}

void DataProcessor::startProcessing()
{
    QMutexLocker locker(&m_mutex);

    if (!m_isProcessing) {
        // 启动处理定时器
        m_processingTimer->start();
        m_isProcessing = true;
        qDebug() << "开始数据处理，线程ID:" << QThread::currentThreadId();
    }
}

void DataProcessor::stopProcessing()
{
    QMutexLocker locker(&m_mutex);

    if (m_isProcessing) {
        // 停止处理定时器
        m_processingTimer->stop();
        m_isProcessing = false;
        qDebug() << "停止数据处理，线程ID:" << QThread::currentThreadId();
    }
}

bool DataProcessor::isProcessing() const
{
    QMutexLocker locker(&m_mutex);
    return m_isProcessing;
}

Core::SynchronizedDataFrame DataProcessor::getLatestSyncFrame() const
{
    QMutexLocker locker(&m_mutex);
    return m_latestSyncFrame;
}

void DataProcessor::clearAllBuffers()
{
    QMutexLocker locker(&m_mutex);

    // 清除原始数据缓存
    m_rawDataCache.clear();
    
    qDebug() << "清除所有数据缓冲区";
}

void DataProcessor::onRawDataPointReceived(QString deviceId, QString hardwareChannel, double rawValue, qint64 timestamp)
{
    QMutexLocker locker(&m_mutex);

    // 更新原始数据缓存
    QPair<QString, QString> key(deviceId, hardwareChannel);
    RawDataPoint dataPoint;
    dataPoint.value = rawValue;
    dataPoint.timestamp = timestamp;
    m_rawDataCache[key] = dataPoint;
    
    qDebug() << "接收原始数据点 - 设备:" << deviceId << "通道:" << hardwareChannel 
             << "值:" << rawValue << "时间戳:" << timestamp
             << "线程ID:" << QThread::currentThreadId();
}

void DataProcessor::onDeviceStatusChanged(QString deviceId, Core::StatusCode status, QString message)
{
    QMutexLocker locker(&m_mutex);

    // 更新所有与该设备关联的通道状态
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        if (it.value()->getDeviceId() == deviceId) {
            it.value()->setStatus(status, message);
        }
    }
    
    qDebug() << "设备状态变化 - 设备:" << deviceId 
             << "状态:" << Core::statusCodeToString(status)
             << "消息:" << message
             << "线程ID:" << QThread::currentThreadId();
}

void DataProcessor::performProcessing()
{
    QMutexLocker locker(&m_mutex);

    // 处理数据并创建同步数据帧
    Core::SynchronizedDataFrame frame = processData();

    // 更新最新的同步数据帧
    m_latestSyncFrame = frame;

    // 发送同步数据帧就绪信号
    emit syncFrameReady(frame);
    
    qDebug() << "执行数据处理 - 时间戳:" << frame.timestamp 
             << "通道数:" << frame.channelData.size()
             << "线程ID:" << QThread::currentThreadId();
}

void DataProcessor::onChannelStatusChanged(QString channelId, Core::StatusCode status, QString message)
{
    // 转发通道状态变化信号
    emit channelStatusChanged(channelId, status, message);
    
    qDebug() << "通道状态变化 - 通道:" << channelId 
             << "状态:" << Core::statusCodeToString(status)
             << "消息:" << message
             << "线程ID:" << QThread::currentThreadId();
}

void DataProcessor::onChannelError(QString channelId, QString errorMsg)
{
    // 转发通道错误信号
    emit errorOccurred("通道 " + channelId + " 错误: " + errorMsg);
    
    qDebug() << "通道错误 - 通道:" << channelId 
             << "错误:" << errorMsg
             << "线程ID:" << QThread::currentThreadId();
}

Channel* DataProcessor::findChannel(const QString& deviceId, const QString& hardwareChannel) const
{
    // 查找与设备ID和硬件通道匹配的通道
    for (auto it = m_channels.constBegin(); it != m_channels.constEnd(); ++it) {
        if (it.value()->getDeviceId() == deviceId &&
            it.value()->getHardwareChannel() == hardwareChannel) {
            return it.value();
        }
    }

    return nullptr;
}

Core::SynchronizedDataFrame DataProcessor::processData()
{
    // 创建同步数据帧，使用当前时间戳
    Core::SynchronizedDataFrame frame(QDateTime::currentMSecsSinceEpoch());

    // 处理每个通道的数据
    for (auto it = m_channels.constBegin(); it != m_channels.constEnd(); ++it) {
        Channel* channel = it.value();
        QString deviceId = channel->getDeviceId();
        QString hardwareChannel = channel->getHardwareChannel();
        
        // 查找该通道对应的原始数据
        QPair<QString, QString> key(deviceId, hardwareChannel);
        auto dataIt = m_rawDataCache.find(key);
        
        if (dataIt != m_rawDataCache.end()) {
            // 处理原始数据
            double rawValue = dataIt.value().value;
            qint64 timestamp = dataIt.value().timestamp;
            
            // 应用通道处理（增益、偏移和校准）
            Core::ProcessedDataPoint processedPoint = channel->processRawData(rawValue, timestamp);
            
            // 添加到同步数据帧
            frame.addChannelData(channel->getChannelId(), processedPoint);
            
            qDebug() << "处理通道数据 - 通道:" << channel->getChannelId() 
                     << "原始值:" << rawValue 
                     << "处理后值:" << processedPoint.value
                     << "线程ID:" << QThread::currentThreadId();
            
            // 发送处理后数据点就绪信号
            emit processedDataPointReady(channel->getChannelId(), processedPoint);
        }
    }

    return frame;
}

} // namespace Processing
