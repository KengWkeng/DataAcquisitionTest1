#include "VirtualDevice.h"
#include <QThread>

namespace Device {

VirtualDevice::VirtualDevice(const Core::VirtualDeviceConfig& config, QObject *parent)
    : AbstractDevice(parent)
    , m_config(config)
    , m_timer(nullptr)
    , m_startTime(QDateTime::currentMSecsSinceEpoch())
    , m_phase(0.0)
{
    // 创建定时器（延迟到线程启动后）
    m_timer = new QTimer();
    m_timer->setParent(this);

    // 连接定时器信号到生成数据点的槽
    connect(m_timer, &QTimer::timeout, this, &VirtualDevice::generateDataPoint);

    // 设置定时器间隔（默认为10ms，即100Hz）
    int interval = 10; // 毫秒
    m_timer->setInterval(interval);

    qDebug() << "创建虚拟设备:" << m_config.instanceName
             << "信号类型:" << m_config.signalType
             << "振幅:" << m_config.amplitude
             << "频率:" << m_config.frequency;
}

VirtualDevice::~VirtualDevice()
{
    // 确保停止采集和断开连接
    stopAcquisition();
    disconnectDevice();

    // 停止并删除定时器
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }

    qDebug() << "销毁虚拟设备:" << m_config.instanceName;
}

bool VirtualDevice::connectDevice()
{
    qDebug() << "正在连接虚拟设备:" << m_config.instanceName << "线程ID:" << QThread::currentThreadId();

    // 虚拟设备总是可以连接
    setStatus(Core::StatusCode::CONNECTED, "虚拟设备已连接");
    return true;
}

bool VirtualDevice::disconnectDevice()
{
    // 确保停止采集
    stopAcquisition();

    // 虚拟设备总是可以断开连接
    setStatus(Core::StatusCode::DISCONNECTED, "虚拟设备已断开连接");
    return true;
}

void VirtualDevice::startAcquisition()
{
    // 只有在已连接状态下才能开始采集
    if (m_status != Core::StatusCode::CONNECTED && m_status != Core::StatusCode::STOPPED) {
        emit errorOccurred(getDeviceId(), "无法开始采集：设备未连接");
        return;
    }

    // 重置开始时间和相位
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    m_phase = 0.0;

    // 确保定时器在当前线程中启动
    QMetaObject::invokeMethod(m_timer, "start", Qt::QueuedConnection);

    setStatus(Core::StatusCode::ACQUIRING, "虚拟设备正在采集数据");
    qDebug() << "虚拟设备" << m_config.instanceName << "开始采集数据";
}

void VirtualDevice::stopAcquisition()
{
    // 停止定时器
    if (m_timer && m_timer->isActive()) {
        QMetaObject::invokeMethod(m_timer, "stop", Qt::QueuedConnection);
    }

    // 只有在采集状态下才需要更新状态
    if (m_status == Core::StatusCode::ACQUIRING) {
        setStatus(Core::StatusCode::STOPPED, "虚拟设备已停止采集");
        qDebug() << "虚拟设备" << m_config.instanceName << "停止采集数据";
    }
}

QString VirtualDevice::getDeviceId() const
{
    return m_config.deviceId;
}

Core::DeviceType VirtualDevice::getDeviceType() const
{
    return Core::DeviceType::VIRTUAL;
}

void VirtualDevice::generateDataPoint()
{
    // 获取当前时间戳
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    // 根据信号类型生成数据
    double rawValue = 0.0;

    if (m_config.signalType.toLower() == "sine") {
        rawValue = generateSineWave();
    } else if (m_config.signalType.toLower() == "square") {
        rawValue = generateSquareWave();
    } else if (m_config.signalType.toLower() == "triangle") {
        rawValue = generateTriangleWave();
    } else if (m_config.signalType.toLower() == "random") {
        rawValue = generateRandomValue();
    } else {
        // 默认使用正弦波
        rawValue = generateSineWave();
    }

    // 应用滤波器
    double filteredValue = applyFilter(rawValue);

    // 发送原始数据点就绪信号
    emit rawDataPointReady(getDeviceId(), "0", filteredValue, timestamp);

    // 更新相位
    double deltaTime = (timestamp - m_startTime) / 1000.0; // 转换为秒
    m_phase = fmod(2.0 * M_PI * m_config.frequency * deltaTime, 2.0 * M_PI);
}

double VirtualDevice::generateSineWave()
{
    // 生成正弦波：amplitude * sin(phase)
    return m_config.amplitude * qSin(m_phase);
}

double VirtualDevice::generateSquareWave()
{
    // 生成方波：amplitude * sign(sin(phase))
    return m_config.amplitude * (qSin(m_phase) >= 0 ? 1.0 : -1.0);
}

double VirtualDevice::generateTriangleWave()
{
    // 生成三角波：amplitude * (2/pi) * asin(sin(phase))
    return m_config.amplitude * (2.0 / M_PI) * qAsin(qSin(m_phase));
}

double VirtualDevice::generateRandomValue()
{
    // 生成随机值：amplitude * random(-1, 1)
    return m_config.amplitude * (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0);
}

} // namespace Device
