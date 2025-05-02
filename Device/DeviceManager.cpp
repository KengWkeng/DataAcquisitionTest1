#include "DeviceManager.h"
#include <QThread>

namespace Device {

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "创建设备管理器";
}

DeviceManager::~DeviceManager()
{
    // 清理设备和线程
    cleanup();

    qDebug() << "销毁设备管理器";
}

bool DeviceManager::createDevices(const QList<Core::DeviceConfig*>& configs)
{
    bool success = true;

    for (const auto& config : configs) {
        if (!config) {
            qDebug() << "跳过空配置";
            continue;
        }

        AbstractDevice* device = nullptr;

        // 根据设备类型创建相应的设备
        switch (config->deviceType) {
            case Core::DeviceType::VIRTUAL: {
                auto virtualConfig = dynamic_cast<Core::VirtualDeviceConfig*>(config);
                if (virtualConfig) {
                    device = new VirtualDevice(*virtualConfig, nullptr);
                }
                break;
            }
            case Core::DeviceType::MODBUS: {
                auto modbusConfig = dynamic_cast<Core::ModbusDeviceConfig*>(config);
                if (modbusConfig) {
                    device = new ModbusDevice(*modbusConfig, nullptr);
                }
                break;
            }
            case Core::DeviceType::DAQ:
                // TODO: 实现DAQ设备创建
                qDebug() << "DAQ设备尚未实现";
                break;
            case Core::DeviceType::ECU:
                // TODO: 实现ECU设备创建
                qDebug() << "ECU设备尚未实现";
                break;
            default:
                qDebug() << "未知设备类型:" << static_cast<int>(config->deviceType);
                break;
        }

        if (device) {
            // 创建设备线程
            if (createDeviceThread(device)) {
                // 添加到设备映射
                m_devices[device->getDeviceId()] = device;

                // 连接设备信号
                connect(device, &AbstractDevice::rawDataPointReady,
                        this, &DeviceManager::rawDataPointReady);
                connect(device, &AbstractDevice::deviceStatusChanged,
                        this, &DeviceManager::deviceStatusChanged);
                connect(device, &AbstractDevice::errorOccurred,
                        this, &DeviceManager::errorOccurred);

                qDebug() << "创建设备成功:" << device->getDeviceId()
                         << "类型:" << Core::deviceTypeToString(device->getDeviceType());
            } else {
                qDebug() << "创建设备线程失败:" << device->getDeviceId();
                delete device;
                success = false;
            }
        } else {
            qDebug() << "创建设备失败，配置ID:" << config->deviceId;
            success = false;
        }
    }

    return success;
}

bool DeviceManager::createVirtualDevices(const QList<Core::VirtualDeviceConfig>& configs)
{
    bool success = true;

    for (const auto& config : configs) {
        // 创建虚拟设备（不设置父对象，以便可以移动到线程）
        VirtualDevice* device = new VirtualDevice(config, nullptr);

        // 创建设备线程
        if (createDeviceThread(device)) {
            // 添加到设备映射
            m_devices[device->getDeviceId()] = device;

            // 连接设备信号
            connect(device, &AbstractDevice::rawDataPointReady,
                    this, &DeviceManager::rawDataPointReady);
            connect(device, &AbstractDevice::deviceStatusChanged,
                    this, &DeviceManager::deviceStatusChanged);
            connect(device, &AbstractDevice::errorOccurred,
                    this, &DeviceManager::errorOccurred);

            qDebug() << "创建虚拟设备成功:" << device->getDeviceId();
        } else {
            qDebug() << "创建虚拟设备线程失败:" << device->getDeviceId();
            delete device;
            success = false;
        }
    }

    return success;
}

bool DeviceManager::createModbusDevices(const QList<Core::ModbusDeviceConfig>& configs)
{
    bool success = true;

    for (const auto& config : configs) {
        // 创建Modbus设备（不设置父对象，以便可以移动到线程）
        ModbusDevice* device = new ModbusDevice(config, nullptr);

        // 创建设备线程
        if (createDeviceThread(device)) {
            // 添加到设备映射
            m_devices[device->getDeviceId()] = device;

            // 连接设备信号
            connect(device, &AbstractDevice::rawDataPointReady,
                    this, &DeviceManager::rawDataPointReady);
            connect(device, &AbstractDevice::deviceStatusChanged,
                    this, &DeviceManager::deviceStatusChanged);
            connect(device, &AbstractDevice::errorOccurred,
                    this, &DeviceManager::errorOccurred);

            qDebug() << "创建Modbus设备成功:" << device->getDeviceId();
        } else {
            qDebug() << "创建Modbus设备线程失败:" << device->getDeviceId();
            delete device;
            success = false;
        }
    }

    return success;
}

bool DeviceManager::startAllDevices()
{
    bool success = true;

    // 首先连接所有设备
    if (!connectAllDevices()) {
        qDebug() << "连接设备失败，无法启动采集";
        return false;
    }

    // 启动所有设备的采集
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        AbstractDevice* device = it.value();
        if (device) {
            // 直接调用startAcquisition方法
            device->startAcquisition();
            qDebug() << "启动设备采集:" << device->getDeviceId();
        } else {
            qDebug() << "设备为空，无法启动采集:" << it.key();
            success = false;
        }
    }

    return success;
}

bool DeviceManager::stopAllDevices()
{
    bool success = true;

    // 停止所有设备的采集
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        AbstractDevice* device = it.value();
        if (device) {
            // 直接调用stopAcquisition方法
            device->stopAcquisition();
            qDebug() << "停止设备采集:" << device->getDeviceId();
        } else {
            qDebug() << "设备为空，无法停止采集:" << it.key();
            success = false;
        }
    }

    return success;
}

bool DeviceManager::connectAllDevices()
{
    bool success = true;

    // 连接所有设备
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        AbstractDevice* device = it.value();
        if (device) {
            // 直接调用connectDevice方法
            bool result = device->connectDevice();

            if (!result) {
                qDebug() << "连接设备失败:" << device->getDeviceId();
                success = false;
            } else {
                qDebug() << "连接设备成功:" << device->getDeviceId();
            }
        } else {
            qDebug() << "设备为空，无法连接:" << it.key();
            success = false;
        }
    }

    return success;
}

bool DeviceManager::disconnectAllDevices()
{
    bool success = true;

    // 断开所有设备
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        AbstractDevice* device = it.value();
        if (device) {
            // 直接调用disconnectDevice方法
            bool result = device->disconnectDevice();

            if (!result) {
                qDebug() << "断开设备失败:" << device->getDeviceId();
                success = false;
            } else {
                qDebug() << "断开设备成功:" << device->getDeviceId();
            }
        } else {
            qDebug() << "设备为空，无法断开:" << it.key();
            success = false;
        }
    }

    return success;
}

AbstractDevice* DeviceManager::getDevice(const QString& deviceId) const
{
    return m_devices.value(deviceId, nullptr);
}

QMap<QString, AbstractDevice*> DeviceManager::getDevices() const
{
    return m_devices;
}

Core::StatusCode DeviceManager::getDeviceStatus(const QString& deviceId) const
{
    AbstractDevice* device = getDevice(deviceId);
    if (device) {
        return device->getStatus();
    }
    return Core::StatusCode::ERROR_CONFIG;
}

bool DeviceManager::createDeviceThread(AbstractDevice* device)
{
    if (!device) {
        qDebug() << "设备为空，无法创建线程";
        return false;
    }

    // 创建线程（不设置父对象，以便可以在cleanup中删除）
    QThread* thread = new QThread();

    // 将设备移动到线程
    qDebug() << "将设备" << device->getDeviceId() << "移动到线程" << thread;
    device->moveToThread(thread);

    // 连接线程信号
    connect(thread, &QThread::finished, device, &QObject::deleteLater);

    // 添加线程启动信号处理
    connect(thread, &QThread::started, [device]() {
        qDebug() << "设备线程已启动，设备ID:" << device->getDeviceId()
                 << "线程ID:" << QThread::currentThreadId();
    });

    // 启动线程
    thread->start();

    // 确保线程已经启动
    if (!thread->isRunning()) {
        qDebug() << "线程启动失败，设备ID:" << device->getDeviceId();
        delete thread;
        return false;
    }

    // 添加到线程映射
    m_deviceThreads[device->getDeviceId()] = thread;

    qDebug() << "创建设备线程成功:" << device->getDeviceId();
    return true;
}

void DeviceManager::cleanup()
{
    // 停止所有设备
    stopAllDevices();

    // 断开所有设备
    disconnectAllDevices();

    // 停止并删除所有线程
    for (auto it = m_deviceThreads.begin(); it != m_deviceThreads.end(); ++it) {
        QThread* thread = it.value();
        if (thread) {
            thread->quit();
            thread->wait();
            delete thread;
        }
    }

    // 清空映射
    m_deviceThreads.clear();
    m_devices.clear();

    qDebug() << "清理设备和线程完成";
}

} // namespace Device
