#include "Channel.h"
#include <QThread>

namespace Processing {

Channel::Channel(const Core::ChannelConfig& config, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_status(Core::StatusCode::OK)
{
    // 初始化最新数据点
    m_latestDataPoint.channelId = m_config.channelId;
    m_latestDataPoint.value = 0.0;
    m_latestDataPoint.timestamp = 0;
    m_latestDataPoint.status = Core::StatusCode::OK;
    m_latestDataPoint.unit = m_config.params.unit;

    qDebug() << "创建通道:" << m_config.channelId
             << "名称:" << m_config.channelName
             << "设备:" << m_config.deviceId
             << "硬件通道:" << m_config.hardwareChannel
             << "线程ID:" << QThread::currentThreadId();
}

Channel::~Channel()
{
    qDebug() << "销毁通道:" << m_config.channelId
             << "线程ID:" << QThread::currentThreadId();
}

Core::ProcessedDataPoint Channel::processRawData(double rawValue, qint64 timestamp)
{
    QMutexLocker locker(&m_mutex);

    // 应用增益和偏移
    double valueWithGainOffset = applyGainOffset(rawValue);

    // 应用校准
    double calibratedValue = applyCalibration(valueWithGainOffset);

    // 创建处理后的数据点
    Core::ProcessedDataPoint dataPoint;
    dataPoint.channelId = m_config.channelId;
    dataPoint.value = calibratedValue;
    dataPoint.timestamp = timestamp;
    dataPoint.status = m_status;
    dataPoint.unit = m_config.params.unit;

    // 更新最新数据点
    m_latestDataPoint = dataPoint;

    // 记录处理信息
    qDebug() << "通道处理数据:" << m_config.channelId
             << "原始值:" << rawValue
             << "处理后值:" << calibratedValue
             << "线程ID:" << QThread::currentThreadId();

    return dataPoint;
}

Core::ProcessedDataPoint Channel::getLatestProcessedDataPoint() const
{
    QMutexLocker locker(&m_mutex);
    return m_latestDataPoint;
}

QString Channel::getChannelId() const
{
    return m_config.channelId;
}

QString Channel::getChannelName() const
{
    return m_config.channelName;
}

QString Channel::getDeviceId() const
{
    return m_config.deviceId;
}

QString Channel::getHardwareChannel() const
{
    return m_config.hardwareChannel;
}

Core::ChannelParams Channel::getParams() const
{
    QMutexLocker locker(&m_mutex);
    return m_config.params;
}

void Channel::setParams(const Core::ChannelParams& params)
{
    QMutexLocker locker(&m_mutex);
    m_config.params = params;
}

Core::StatusCode Channel::getStatus() const
{
    QMutexLocker locker(&m_mutex);
    return m_status;
}

void Channel::setStatus(Core::StatusCode status, const QString& message)
{
    QMutexLocker locker(&m_mutex);

    if (m_status != status || m_statusMessage != message) {
        m_status = status;
        m_statusMessage = message;

        // 更新最新数据点的状态
        m_latestDataPoint.status = status;

        // 发送状态变化信号
        emit channelStatusChanged(m_config.channelId, m_status, m_statusMessage);

        // 记录状态变化
        qDebug() << "通道" << m_config.channelId << "状态变为"
                 << Core::statusCodeToString(m_status) << ":" << m_statusMessage
                 << "线程ID:" << QThread::currentThreadId();
    }
}

double Channel::applyGainOffset(double rawValue) const
{
    // 应用增益和偏移: value * gain + offset
    return rawValue * m_config.params.gain + m_config.params.offset;
}

double Channel::applyCalibration(double value) const
{
    // 应用校准多项式: a*x^3 + b*x^2 + c*x + d
    const Core::CalibrationParams& params = m_config.params.calibrationParams;
    return params.a * value * value * value +
           params.b * value * value +
           params.c * value +
           params.d;
}



} // namespace Processing
