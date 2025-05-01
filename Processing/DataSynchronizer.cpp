#include "DataSynchronizer.h"
#include <QDateTime>

namespace Processing {

DataSynchronizer::DataSynchronizer(int syncIntervalMs, QObject *parent)
    : QObject(parent)
    , m_syncTimer(new QTimer(this))
    , m_syncIntervalMs(syncIntervalMs)
    , m_isSyncing(false)
{
    // 连接同步定时器信号
    connect(m_syncTimer, &QTimer::timeout, this, &DataSynchronizer::performSync);

    // 设置同步定时器间隔
    m_syncTimer->setInterval(m_syncIntervalMs);

    qDebug() << "创建数据同步器，同步间隔:" << m_syncIntervalMs << "毫秒";
}

DataSynchronizer::~DataSynchronizer()
{
    // 停止同步
    stopSync();

    // 清理通道
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        delete it.value();
    }
    m_channels.clear();

    qDebug() << "销毁数据同步器";
}

bool DataSynchronizer::createChannel(const Core::ChannelConfig& config)
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
            this, &DataSynchronizer::onProcessedDataPointReady);
    connect(channel, &Channel::channelStatusChanged,
            this, &DataSynchronizer::onChannelStatusChanged);
    connect(channel, &Channel::errorOccurred,
            this, &DataSynchronizer::onChannelError);

    // 添加到通道映射
    m_channels[config.channelId] = channel;

    qDebug() << "创建通道成功:" << config.channelId;
    return true;
}

bool DataSynchronizer::createChannels(const QMap<QString, Core::ChannelConfig>& configs)
{
    bool success = true;

    for (auto it = configs.constBegin(); it != configs.constEnd(); ++it) {
        if (!createChannel(it.value())) {
            success = false;
        }
    }

    return success;
}

Channel* DataSynchronizer::getChannel(const QString& channelId) const
{
    QMutexLocker locker(&m_mutex);
    return m_channels.value(channelId, nullptr);
}

QMap<QString, Channel*> DataSynchronizer::getChannels() const
{
    QMutexLocker locker(&m_mutex);
    return m_channels;
}

int DataSynchronizer::getSyncIntervalMs() const
{
    return m_syncIntervalMs;
}

void DataSynchronizer::setSyncIntervalMs(int intervalMs)
{
    if (intervalMs > 0 && intervalMs != m_syncIntervalMs) {
        m_syncIntervalMs = intervalMs;
        m_syncTimer->setInterval(m_syncIntervalMs);
        qDebug() << "设置同步间隔为:" << m_syncIntervalMs << "毫秒";
    }
}

void DataSynchronizer::startSync()
{
    QMutexLocker locker(&m_mutex);

    if (!m_isSyncing) {
        // 启动同步定时器
        m_syncTimer->start();
        m_isSyncing = true;
        qDebug() << "开始数据同步";
    }
}

void DataSynchronizer::stopSync()
{
    QMutexLocker locker(&m_mutex);

    if (m_isSyncing) {
        // 停止同步定时器
        m_syncTimer->stop();
        m_isSyncing = false;
        qDebug() << "停止数据同步";
    }
}

bool DataSynchronizer::isSyncing() const
{
    QMutexLocker locker(&m_mutex);
    return m_isSyncing;
}

Core::SynchronizedDataFrame DataSynchronizer::getLatestSyncFrame() const
{
    QMutexLocker locker(&m_mutex);
    return m_latestSyncFrame;
}

void DataSynchronizer::clearAllBuffers()
{
    QMutexLocker locker(&m_mutex);

    // 由于通道不再有缓冲区，此方法现在只是一个空操作
    qDebug() << "清除所有通道的数据缓冲区（空操作）";
}

void DataSynchronizer::onRawDataPointReceived(QString deviceId, QString hardwareChannel, double rawValue, qint64 timestamp)
{
    // 查找对应的通道
    Channel* channel = findChannel(deviceId, hardwareChannel);

    if (channel) {
        // 处理原始数据点
        channel->processRawData(rawValue, timestamp);
    } else {
        qDebug() << "未找到对应的通道，设备ID:" << deviceId << "硬件通道:" << hardwareChannel;
    }
}

void DataSynchronizer::onDeviceStatusChanged(QString deviceId, Core::StatusCode status, QString message)
{
    QMutexLocker locker(&m_mutex);

    // 更新所有与该设备关联的通道状态
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        if (it.value()->getDeviceId() == deviceId) {
            it.value()->setStatus(status, message);
        }
    }
}

void DataSynchronizer::performSync()
{
    QMutexLocker locker(&m_mutex);

    // 创建同步数据帧
    Core::SynchronizedDataFrame frame = createSyncFrame();

    // 更新最新的同步数据帧
    m_latestSyncFrame = frame;

    // 发送同步数据帧就绪信号
    emit syncFrameReady(frame);
}

void DataSynchronizer::onChannelStatusChanged(QString channelId, Core::StatusCode status, QString message)
{
    // 转发通道状态变化信号
    emit channelStatusChanged(channelId, status, message);
}

void DataSynchronizer::onChannelError(QString channelId, QString errorMsg)
{
    // 转发通道错误信号
    emit errorOccurred("通道 " + channelId + " 错误: " + errorMsg);
}

void DataSynchronizer::onProcessedDataPointReady(QString channelId, Core::ProcessedDataPoint dataPoint)
{
    // 转发处理后数据点就绪信号
    emit processedDataPointReady(channelId, dataPoint);
}

Channel* DataSynchronizer::findChannel(const QString& deviceId, const QString& hardwareChannel) const
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

Core::SynchronizedDataFrame DataSynchronizer::createSyncFrame() const
{
    // 创建同步数据帧
    Core::SynchronizedDataFrame frame(QDateTime::currentMSecsSinceEpoch());

    // 添加所有通道的最新数据点
    for (auto it = m_channels.constBegin(); it != m_channels.constEnd(); ++it) {
        Core::ProcessedDataPoint dataPoint = it.value()->getLatestProcessedDataPoint();
        frame.addChannelData(it.key(), dataPoint);
    }

    return frame;
}

} // namespace Processing
