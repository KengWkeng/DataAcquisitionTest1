#include "ModbusDevice.h"
#include <QThread>

namespace Device {

ModbusDevice::ModbusDevice(const Core::ModbusDeviceConfig& config, QObject *parent)
    : AbstractDevice(parent)
    , m_config(config)
    , m_modbusClient(nullptr)
    , m_timer(nullptr)
    , m_isAcquiring(false)
{
    // 注意：不在构造函数中创建QModbusRtuSerialClient，而是在线程启动后创建
    // 这样可以确保QModbusRtuSerialClient和QSerialPort对象在正确的线程中创建

    // 创建定时器（延迟到线程启动后）
    m_timer = new QTimer(this);

    // 连接定时器信号到读取数据的槽
    connect(m_timer, &QTimer::timeout, this, &ModbusDevice::readModbusData);

    // 设置定时器间隔
    m_timer->setInterval(m_config.readCycleMs);

    // 初始化通道映射
    for (const auto& slave : m_config.slaves) {
        for (const auto& reg : slave.registers) {
            m_channelNames[slave.slaveId][reg.registerAddress] = reg.channelName;
            m_channelParams[slave.slaveId][reg.registerAddress] = reg.channelParams;
        }
    }

    // 连接线程启动信号，确保在正确的线程中创建QModbusRtuSerialClient
    connect(QThread::currentThread(), &QThread::started, this, &ModbusDevice::initializeModbusClient);

    qDebug() << "创建Modbus设备:" << m_config.instanceName
             << "串口:" << m_config.serialConfig.port
             << "波特率:" << m_config.serialConfig.baudrate
             << "从站数量:" << m_config.slaves.size();
}

ModbusDevice::~ModbusDevice()
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

    // 删除Modbus客户端
    if (m_modbusClient) {
        delete m_modbusClient;
        m_modbusClient = nullptr;
    }

    qDebug() << "销毁Modbus设备:" << m_config.instanceName;
}

void ModbusDevice::initializeModbusClient()
{
    // 确保在正确的线程中创建QModbusRtuSerialClient
    if (m_modbusClient) {
        // 如果已经存在，先删除
        delete m_modbusClient;
        m_modbusClient = nullptr;
    }

    // 创建新的Modbus客户端
    m_modbusClient = new QModbusRtuSerialClient(this);

    qDebug() << "初始化Modbus客户端 - 设备:" << m_config.instanceName
             << "线程ID:" << QThread::currentThreadId();
}

bool ModbusDevice::connectDevice()
{
    qDebug() << "正在连接Modbus设备:" << m_config.instanceName
             << "串口:" << m_config.serialConfig.port
             << "波特率:" << m_config.serialConfig.baudrate
             << "线程ID:" << QThread::currentThreadId();

    // 检查Modbus客户端是否已初始化
    if (!m_modbusClient) {
        // 如果未初始化，在当前线程中初始化
        initializeModbusClient();

        if (!m_modbusClient) {
            setStatus(Core::StatusCode::ERROR_CONFIG, "Modbus客户端初始化失败");
            return false;
        }
    }

    // 如果已经连接，直接返回成功
    if (m_modbusClient->state() == QModbusDevice::ConnectedState) {
        qDebug() << "Modbus设备已连接，无需重新连接:" << m_config.instanceName;
        setStatus(Core::StatusCode::CONNECTED, "Modbus设备已连接");
        return true;
    }

    // 如果设备正在连接或关闭中，等待状态变化
    if (m_modbusClient->state() == QModbusDevice::ConnectingState ||
        m_modbusClient->state() == QModbusDevice::ClosingState) {
        qDebug() << "Modbus设备正在状态转换中，等待完成:" << m_config.instanceName
                 << "当前状态:" << m_modbusClient->state();

        // 等待一段时间让状态转换完成
        QThread::msleep(100);

        // 如果转换为已连接状态，返回成功
        if (m_modbusClient->state() == QModbusDevice::ConnectedState) {
            setStatus(Core::StatusCode::CONNECTED, "Modbus设备已连接");
            return true;
        }
    }

    // 如果设备处于未连接状态以外的其他状态，先断开连接
    if (m_modbusClient->state() != QModbusDevice::UnconnectedState) {
        qDebug() << "Modbus设备状态异常，先断开连接:" << m_config.instanceName
                 << "当前状态:" << m_modbusClient->state();
        m_modbusClient->disconnectDevice();

        // 等待断开连接完成
        QThread::msleep(100);
    }

    // 配置串口参数
    if (!configureSerialPort()) {
        QString errorMsg = "串口配置失败: " + m_config.serialConfig.port;
        qDebug() << errorMsg;
        setStatus(Core::StatusCode::ERROR_CONFIG, errorMsg);
        return false;
    }

    // 连接设备
    if (!m_modbusClient->connectDevice()) {
        QString errorMsg = QString("Modbus连接失败: %1 - %2").arg(m_config.serialConfig.port).arg(m_modbusClient->errorString());
        qDebug() << errorMsg;
        setStatus(Core::StatusCode::ERROR_CONNECTION, errorMsg);
        return false;
    }

    qDebug() << "Modbus设备连接成功:" << m_config.instanceName
             << "串口:" << m_config.serialConfig.port;
    setStatus(Core::StatusCode::CONNECTED, "Modbus设备已连接");
    return true;
}

bool ModbusDevice::disconnectDevice()
{
    // 确保停止采集
    stopAcquisition();

    // 检查客户端是否存在
    if (!m_modbusClient) {
        qDebug() << "Modbus客户端为空，无需断开连接:" << m_config.instanceName;
        setStatus(Core::StatusCode::DISCONNECTED, "Modbus设备已断开连接");
        return true;
    }

    // 如果已经断开连接，直接返回成功
    if (m_modbusClient->state() == QModbusDevice::UnconnectedState) {
        qDebug() << "Modbus设备已断开连接，无需重复操作:" << m_config.instanceName;
        setStatus(Core::StatusCode::DISCONNECTED, "Modbus设备已断开连接");
        return true;
    }

    // 如果设备正在关闭中，等待状态变化
    if (m_modbusClient->state() == QModbusDevice::ClosingState) {
        qDebug() << "Modbus设备正在关闭中，等待完成:" << m_config.instanceName;

        // 等待一段时间让状态转换完成
        QThread::msleep(100);

        // 如果转换为已断开状态，返回成功
        if (m_modbusClient->state() == QModbusDevice::UnconnectedState) {
            setStatus(Core::StatusCode::DISCONNECTED, "Modbus设备已断开连接");
            return true;
        }
    }

    // 断开连接
    qDebug() << "正在断开Modbus设备连接:" << m_config.instanceName
             << "当前状态:" << m_modbusClient->state();
    m_modbusClient->disconnectDevice();

    // 等待断开连接完成
    QThread::msleep(50);

    setStatus(Core::StatusCode::DISCONNECTED, "Modbus设备已断开连接");
    return true;
}

void ModbusDevice::startAcquisition()
{
    // 只有在已连接状态下才能开始采集
    if (m_status != Core::StatusCode::CONNECTED && m_status != Core::StatusCode::STOPPED) {
        qDebug() << "设备未连接，尝试连接设备:" << getDeviceId();

        // 尝试连接设备
        if (!connectDevice()) {
            emit errorOccurred(getDeviceId(), "无法开始采集：设备连接失败");
            return;
        }
    }

    QMutexLocker locker(&m_mutex);

    // 如果已经在采集，不要重复启动
    if (m_isAcquiring) {
        qDebug() << "设备已经在采集数据，忽略重复启动:" << getDeviceId();
        return;
    }

    m_isAcquiring = true;

    // 确保定时器在当前线程中启动
    QMetaObject::invokeMethod(m_timer, "start", Qt::QueuedConnection);

    setStatus(Core::StatusCode::ACQUIRING, "Modbus设备正在采集数据");
    qDebug() << "Modbus设备" << m_config.instanceName << "开始采集数据";
}

void ModbusDevice::stopAcquisition()
{
    QMutexLocker locker(&m_mutex);
    m_isAcquiring = false;

    // 停止定时器
    if (m_timer && m_timer->isActive()) {
        QMetaObject::invokeMethod(m_timer, "stop", Qt::QueuedConnection);
    }

    // 只有在采集状态下才需要更新状态
    if (m_status == Core::StatusCode::ACQUIRING) {
        setStatus(Core::StatusCode::STOPPED, "Modbus设备已停止采集");
        qDebug() << "Modbus设备" << m_config.instanceName << "停止采集数据";
    }
}

QString ModbusDevice::getDeviceId() const
{
    return m_config.deviceId;
}

Core::DeviceType ModbusDevice::getDeviceType() const
{
    return Core::DeviceType::MODBUS;
}

void ModbusDevice::readModbusData()
{
    QMutexLocker locker(&m_mutex);

    // 检查是否正在采集
    if (!m_isAcquiring) {
        return;
    }

    // 检查客户端状态
    if (!m_modbusClient) {
        QString errorMsg = "Modbus客户端为空";
        qDebug() << errorMsg << "设备:" << getDeviceId();
        emit errorOccurred(getDeviceId(), errorMsg);

        // 尝试初始化Modbus客户端
        initializeModbusClient();

        if (!m_modbusClient) {
            qDebug() << "初始化Modbus客户端失败:" << getDeviceId();
            return;
        }
    }

    // 检查客户端状态，但避免频繁重连
    if (m_modbusClient->state() != QModbusDevice::ConnectedState) {
        // 如果设备正在关闭或连接中，等待状态变化
        if (m_modbusClient->state() == QModbusDevice::ClosingState ||
            m_modbusClient->state() == QModbusDevice::ConnectingState) {
            qDebug() << "Modbus设备正在状态转换中，等待完成:" << getDeviceId()
                     << "当前状态:" << m_modbusClient->state();
            return;
        }

        // 如果设备已断开，尝试重新连接
        QString errorMsg = QString("Modbus客户端未连接，当前状态: %1").arg(m_modbusClient->state());
        qDebug() << errorMsg << "设备:" << getDeviceId();
        emit errorOccurred(getDeviceId(), errorMsg);

        // 尝试重新连接，但避免频繁重连
        static QDateTime lastReconnectAttempt;
        QDateTime now = QDateTime::currentDateTime();

        // 至少间隔5秒再尝试重连
        if (lastReconnectAttempt.isNull() || lastReconnectAttempt.msecsTo(now) > 5000) {
            qDebug() << "尝试重新连接Modbus设备:" << getDeviceId();
            lastReconnectAttempt = now;

            if (connectDevice()) {
                qDebug() << "重新连接Modbus设备成功:" << getDeviceId();
            } else {
                qDebug() << "重新连接Modbus设备失败:" << getDeviceId();
                return;
            }
        } else {
            qDebug() << "重连尝试过于频繁，跳过本次重连:" << getDeviceId();
            return;
        }
    }

    // 遍历所有从站
    for (const auto& slave : m_config.slaves) {
        // 获取该从站的所有寄存器地址
        QList<int> registerAddresses;
        for (const auto& reg : slave.registers) {
            registerAddresses.append(reg.registerAddress);
        }

        // 如果没有寄存器，跳过该从站
        if (registerAddresses.isEmpty()) {
            continue;
        }

        // 对寄存器地址进行排序
        std::sort(registerAddresses.begin(), registerAddresses.end());

        // 合并连续的寄存器地址，减少请求次数
        int startAddress = registerAddresses.first();
        int count = 1;

        for (int i = 1; i < registerAddresses.size(); ++i) {
            if (registerAddresses[i] == registerAddresses[i-1] + 1) {
                // 连续的寄存器
                count++;
            } else {
                // 不连续，发送请求并重新开始
                QModbusReply* reply = readRegisters(slave.slaveId, slave.operationCommand,
                                                  startAddress, count);
                if (reply) {
                    connect(reply, &QModbusReply::finished, this, [this, reply]() {
                        this->processModbusResponse(reply);
                    });
                } else {
                    qDebug() << "发送Modbus请求失败 - 从站:" << slave.slaveId
                             << "功能码:" << slave.operationCommand
                             << "起始地址:" << startAddress
                             << "数量:" << count
                             << "设备:" << getDeviceId();
                }

                startAddress = registerAddresses[i];
                count = 1;
            }
        }

        // 发送最后一组请求
        QModbusReply* reply = readRegisters(slave.slaveId, slave.operationCommand,
                                          startAddress, count);
        if (reply) {
            connect(reply, &QModbusReply::finished, this, [this, reply]() {
                this->processModbusResponse(reply);
            });
        } else {
            qDebug() << "发送Modbus请求失败(最后一组) - 从站:" << slave.slaveId
                     << "功能码:" << slave.operationCommand
                     << "起始地址:" << startAddress
                     << "数量:" << count
                     << "设备:" << getDeviceId();
        }
    }
}

void ModbusDevice::processModbusResponse(QModbusReply *reply)
{
    // 确保响应对象有效
    if (!reply) {
        qDebug() << "Modbus响应对象为空，设备:" << getDeviceId();
        return;
    }

    // 设置自动删除，确保资源释放
    reply->setParent(this);

    // 检查是否有错误
    if (reply->error() != QModbusDevice::NoError) {
        QString errorMsg = QString("Modbus响应错误: %1 - 从站: %2").arg(reply->errorString()).arg(reply->serverAddress());
        qDebug() << errorMsg << "设备:" << getDeviceId();
        emit errorOccurred(getDeviceId(), errorMsg);
        reply->deleteLater();
        return;
    }

    // 获取响应数据
    const QModbusDataUnit unit = reply->result();
    int slaveId = reply->serverAddress();
    int startAddress = unit.startAddress();
    int valueCount = unit.valueCount();

    qDebug() << "收到Modbus响应 - 从站:" << slaveId
             << "起始地址:" << startAddress
             << "值数量:" << valueCount
             << "设备:" << getDeviceId();

    // 获取当前时间戳
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    // 处理每个寄存器的值
    for (uint i = 0; i < valueCount; ++i) {
        int registerAddress = startAddress + i;
        quint16 rawValue = unit.value(i);

        // 获取通道名称和参数
        QString channelName = getRegisterChannelName(slaveId, registerAddress);
        Core::ChannelParams params = getRegisterChannelParams(slaveId, registerAddress);

        // 如果找不到通道名称，跳过该寄存器
        if (channelName.isEmpty()) {
            continue;
        }

        // 应用滤波器
        double filteredValue = applyFilter(static_cast<double>(rawValue));

        // 构造硬件通道标识（从站ID_寄存器地址）
        QString hardwareChannel = QString("%1_%2").arg(slaveId).arg(registerAddress);

        // 发送原始数据点就绪信号
        emit rawDataPointReady(getDeviceId(), hardwareChannel, filteredValue, timestamp);

        qDebug() << "Modbus数据点 - 设备:" << getDeviceId()
                 << "从站:" << slaveId
                 << "地址:" << registerAddress
                 << "通道:" << channelName
                 << "值:" << filteredValue;
    }

    // 删除响应对象
    reply->deleteLater();
}

bool ModbusDevice::configureSerialPort()
{
    qDebug() << "配置Modbus串口 - 设备:" << getDeviceId()
             << "串口:" << m_config.serialConfig.port
             << "波特率:" << m_config.serialConfig.baudrate;

    // 设置串口名称
    m_modbusClient->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                                         m_config.serialConfig.port);

    // 设置波特率
    m_modbusClient->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
                                         m_config.serialConfig.baudrate);

    // 设置数据位
    QSerialPort::DataBits dataBits;
    switch (m_config.serialConfig.databits) {
        case 5: dataBits = QSerialPort::Data5; break;
        case 6: dataBits = QSerialPort::Data6; break;
        case 7: dataBits = QSerialPort::Data7; break;
        case 8: default: dataBits = QSerialPort::Data8; break;
    }
    m_modbusClient->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, dataBits);

    // 设置停止位
    QSerialPort::StopBits stopBits;
    switch (m_config.serialConfig.stopbits) {
        case 2: stopBits = QSerialPort::TwoStop; break;
        case 3: stopBits = QSerialPort::OneAndHalfStop; break;
        case 1: default: stopBits = QSerialPort::OneStop; break;
    }
    m_modbusClient->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, stopBits);

    // 设置校验位
    QSerialPort::Parity parity;
    if (m_config.serialConfig.parity == "E") {
        parity = QSerialPort::EvenParity;
    } else if (m_config.serialConfig.parity == "O") {
        parity = QSerialPort::OddParity;
    } else if (m_config.serialConfig.parity == "S") {
        parity = QSerialPort::SpaceParity;
    } else if (m_config.serialConfig.parity == "M") {
        parity = QSerialPort::MarkParity;
    } else {
        parity = QSerialPort::NoParity;
    }
    m_modbusClient->setConnectionParameter(QModbusDevice::SerialParityParameter, parity);

    // 设置响应超时和重试次数
    int timeout = 1000; // 1秒超时
    int retries = 3;    // 3次重试

    m_modbusClient->setTimeout(timeout);
    m_modbusClient->setNumberOfRetries(retries);

    // 检查可用的串口
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    qDebug() << "系统可用串口数量:" << ports.size();

    bool portFound = false;
    for (const QSerialPortInfo& info : ports) {
        qDebug() << "可用串口:" << info.portName()
                 << "描述:" << info.description();

        if (info.portName() == m_config.serialConfig.port) {
            portFound = true;
            qDebug() << "找到匹配的串口:" << info.portName();
        }
    }

    if (!portFound) {
        qDebug() << "警告: 未找到配置的串口:" << m_config.serialConfig.port << "设备:" << getDeviceId();
    }

    return true;
}

QModbusReply* ModbusDevice::readRegisters(int slaveId, int functionCode, int startAddress, int count)
{
    // 创建数据单元
    QModbusDataUnit::RegisterType registerType;

    // 根据功能码确定寄存器类型
    switch (functionCode) {
        case 1:
            registerType = QModbusDataUnit::Coils;
            break;
        case 2:
            registerType = QModbusDataUnit::DiscreteInputs;
            break;
        case 3:
            registerType = QModbusDataUnit::HoldingRegisters;
            break;
        case 4:
            registerType = QModbusDataUnit::InputRegisters;
            break;
        default:
            qDebug() << "不支持的功能码:" << functionCode << "设备:" << getDeviceId();
            return nullptr;
    }

    // 创建数据单元 - 注意：某些设备可能需要调整寄存器地址
    // 例如，有些设备使用0-based寻址，有些使用1-based寻址
    // 如果设备使用1-based寻址但配置文件中使用0-based，则需要加1
    // 如果设备使用0-based寻址但配置文件中使用1-based，则需要减1

    // 这里我们假设配置文件中的地址是0-based，与Qt API一致
    QModbusDataUnit dataUnit(registerType, startAddress, count);

    // 发送读取请求
    QModbusReply* reply = m_modbusClient->sendReadRequest(dataUnit, slaveId);

    // 检查请求是否成功
    if (!reply) {
        QString errorMsg = QString("发送Modbus请求失败: %1 - 从站: %2").arg(m_modbusClient->errorString()).arg(slaveId);
        qDebug() << errorMsg << "设备:" << getDeviceId();
        emit errorOccurred(getDeviceId(), errorMsg);
        return nullptr;
    }

    if (reply->isFinished()) {
        // 请求已完成（可能是错误）
        if (reply->error() != QModbusDevice::NoError) {
            QString errorMsg = QString("Modbus请求错误: %1 - 从站: %2").arg(reply->errorString()).arg(slaveId);
            qDebug() << errorMsg << "设备:" << getDeviceId();
            emit errorOccurred(getDeviceId(), errorMsg);
        } else {
            // 如果没有错误，可能是请求成功完成
            processModbusResponse(reply);
        }

        reply->deleteLater();
        return nullptr;
    }

    return reply;
}

QString ModbusDevice::getRegisterChannelName(int slaveId, int registerAddress) const
{
    // 检查从站ID是否存在
    if (!m_channelNames.contains(slaveId)) {
        return QString();
    }

    // 检查寄存器地址是否存在
    if (!m_channelNames[slaveId].contains(registerAddress)) {
        return QString();
    }

    return m_channelNames[slaveId][registerAddress];
}

Core::ChannelParams ModbusDevice::getRegisterChannelParams(int slaveId, int registerAddress) const
{
    // 检查从站ID是否存在
    if (!m_channelParams.contains(slaveId)) {
        return Core::ChannelParams();
    }

    // 检查寄存器地址是否存在
    if (!m_channelParams[slaveId].contains(registerAddress)) {
        return Core::ChannelParams();
    }

    return m_channelParams[slaveId][registerAddress];
}

} // namespace Device
