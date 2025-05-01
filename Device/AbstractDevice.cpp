#include "AbstractDevice.h"

namespace Device {

AbstractDevice::AbstractDevice(QObject *parent)
    : QObject(parent)
    , m_status(Core::StatusCode::DISCONNECTED)
{
}

AbstractDevice::~AbstractDevice()
{
}

Core::StatusCode AbstractDevice::getStatus() const
{
    return m_status;
}

double AbstractDevice::applyFilter(double rawValue)
{
    // 默认实现不进行滤波，直接返回原始值
    // 子类可以重写此方法实现特定的滤波算法
    return rawValue;
}

void AbstractDevice::setStatus(Core::StatusCode status, const QString& message)
{
    if (m_status != status || m_statusMessage != message) {
        m_status = status;
        m_statusMessage = message;
        
        // 发送状态变化信号
        emit deviceStatusChanged(getDeviceId(), m_status, m_statusMessage);
        
        // 记录状态变化
        qDebug() << "设备" << getDeviceId() << "状态变为" 
                 << Core::statusCodeToString(m_status) << ":" << m_statusMessage;
    }
}

} // namespace Device
