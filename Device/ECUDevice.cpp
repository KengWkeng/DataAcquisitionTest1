#include "ECUDevice.h"
#include <QDebug>
#include <QThread>

namespace Device {

ECUDevice::ECUDevice(const Core::ECUDeviceConfig& config, QObject *parent)
    : AbstractDevice(parent)
    , m_config(config)
    , m_serialPort(nullptr)
    , m_timer(nullptr)
    , m_isAcquiring(false)
{
    qDebug() << "[ECUDevice] 构造函数开始，设备:" << config.instanceName
             << "线程ID:" << QThread::currentThreadId();

    // 注意：不在构造函数中创建QSerialPort，而是在线程启动后创建
    // 这样可以确保QSerialPort对象在正确的线程中创建

    // 创建定时器（延迟到线程启动后）
    m_timer = new QTimer(this);

    // 连接定时器信号到读取数据的槽
    connect(m_timer, &QTimer::timeout, this, &ECUDevice::readECUData);

    // 设置定时器间隔
    m_timer->setInterval(m_config.readCycleMs);
    qDebug() << "[ECUDevice] 定时器间隔设置为:" << m_config.readCycleMs << "毫秒";

    // 初始化通道参数映射
    for (auto it = m_config.channels.begin(); it != m_config.channels.end(); ++it) {
        m_channelParams[it.key()] = it.value().channelParams;
        qDebug() << "[ECUDevice] 添加通道参数映射:" << it.key()
                 << "增益:" << it.value().channelParams.gain
                 << "偏移:" << it.value().channelParams.offset;
    }

    // 连接线程启动信号，确保在正确的线程中创建QSerialPort
    connect(QThread::currentThread(), &QThread::started, this, &ECUDevice::initializeSerialPort);
    qDebug() << "[ECUDevice] 已连接线程启动信号到initializeSerialPort槽";

    qDebug() << "[ECUDevice] 创建ECU设备完成:" << m_config.instanceName
             << "串口:" << m_config.serialConfig.port
             << "波特率:" << m_config.serialConfig.baudrate
             << "通道数量:" << m_config.channels.size()
             << "线程ID:" << QThread::currentThreadId();
}

ECUDevice::~ECUDevice()
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

    // 删除串口对象
    if (m_serialPort) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }
        delete m_serialPort;
        m_serialPort = nullptr;
    }

    qDebug() << "销毁ECU设备:" << m_config.instanceName;
}

void ECUDevice::initializeSerialPort()
{
    qDebug() << "[ECUDevice] 开始初始化串口对象，设备:" << m_config.instanceName
             << "线程ID:" << QThread::currentThreadId();

    QMutexLocker locker(&m_mutex);

    // 如果串口对象已存在，先删除
    if (m_serialPort) {
        qDebug() << "[ECUDevice] 串口对象已存在，先关闭并删除";
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }
        delete m_serialPort;
    }

    // 创建新的串口对象
    m_serialPort = new QSerialPort(this);
    qDebug() << "[ECUDevice] 创建新的串口对象，父对象:" << (this ? "有效" : "无效");

    // 连接串口信号
    connect(m_serialPort, &QSerialPort::readyRead, this, &ECUDevice::handleSerialData);
    qDebug() << "[ECUDevice] 已连接串口readyRead信号到handleSerialData槽";

    // 列出系统可用的串口
    QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();
    qDebug() << "[ECUDevice] 系统可用串口列表:";
    for (const QSerialPortInfo &info : availablePorts) {
        qDebug() << "  - 端口名:" << info.portName()
                 << "描述:" << info.description()
                 << "制造商:" << info.manufacturer();
    }

    qDebug() << "[ECUDevice] ECU设备串口对象初始化完成:" << m_config.instanceName
             << "目标串口:" << m_config.serialConfig.port
             << "线程ID:" << QThread::currentThreadId();
}

bool ECUDevice::configureSerialPort()
{
    if (!m_serialPort) {
        qDebug() << "串口对象未初始化";
        return false;
    }

    // 设置串口名称
    m_serialPort->setPortName(m_config.serialConfig.port);

    // 设置波特率
    switch (m_config.serialConfig.baudrate) {
        case 1200: m_serialPort->setBaudRate(QSerialPort::Baud1200); break;
        case 2400: m_serialPort->setBaudRate(QSerialPort::Baud2400); break;
        case 4800: m_serialPort->setBaudRate(QSerialPort::Baud4800); break;
        case 9600: m_serialPort->setBaudRate(QSerialPort::Baud9600); break;
        case 19200: m_serialPort->setBaudRate(QSerialPort::Baud19200); break;
        case 38400: m_serialPort->setBaudRate(QSerialPort::Baud38400); break;
        case 57600: m_serialPort->setBaudRate(QSerialPort::Baud57600); break;
        case 115200: m_serialPort->setBaudRate(QSerialPort::Baud115200); break;
        default:
            qDebug() << "不支持的波特率:" << m_config.serialConfig.baudrate;
            m_serialPort->setBaudRate(QSerialPort::Baud9600);
            break;
    }

    // 设置数据位
    switch (m_config.serialConfig.databits) {
        case 5: m_serialPort->setDataBits(QSerialPort::Data5); break;
        case 6: m_serialPort->setDataBits(QSerialPort::Data6); break;
        case 7: m_serialPort->setDataBits(QSerialPort::Data7); break;
        case 8: m_serialPort->setDataBits(QSerialPort::Data8); break;
        default:
            qDebug() << "不支持的数据位:" << m_config.serialConfig.databits;
            m_serialPort->setDataBits(QSerialPort::Data8);
            break;
    }

    // 设置停止位
    switch (m_config.serialConfig.stopbits) {
        case 1: m_serialPort->setStopBits(QSerialPort::OneStop); break;
        case 2: m_serialPort->setStopBits(QSerialPort::TwoStop); break;
        default:
            qDebug() << "不支持的停止位:" << m_config.serialConfig.stopbits;
            m_serialPort->setStopBits(QSerialPort::OneStop);
            break;
    }

    // 设置校验位
    if (m_config.serialConfig.parity.toUpper() == "N") {
        m_serialPort->setParity(QSerialPort::NoParity);
    } else if (m_config.serialConfig.parity.toUpper() == "E") {
        m_serialPort->setParity(QSerialPort::EvenParity);
    } else if (m_config.serialConfig.parity.toUpper() == "O") {
        m_serialPort->setParity(QSerialPort::OddParity);
    } else {
        qDebug() << "不支持的校验位:" << m_config.serialConfig.parity;
        m_serialPort->setParity(QSerialPort::NoParity);
    }

    // 设置流控制
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    return true;
}

bool ECUDevice::connectDevice()
{
    qDebug() << "正在连接ECU设备:" << m_config.instanceName
             << "串口:" << m_config.serialConfig.port
             << "波特率:" << m_config.serialConfig.baudrate
             << "线程ID:" << QThread::currentThreadId();

    // 检查串口对象是否已初始化
    if (!m_serialPort) {
        qDebug() << "[ECUDevice] 串口对象未初始化，尝试初始化...";
        // 如果未初始化，在当前线程中初始化
        initializeSerialPort();

        if (!m_serialPort) {
            qDebug() << "[ECUDevice] 串口对象初始化失败!";
            setStatus(Core::StatusCode::ERROR_CONFIG, "串口对象初始化失败");
            return false;
        }
        qDebug() << "[ECUDevice] 串口对象初始化成功";
    }

    // 如果已经连接，直接返回成功
    if (m_serialPort->isOpen()) {
        qDebug() << "ECU设备已连接，无需重新连接:" << m_config.instanceName;
        setStatus(Core::StatusCode::CONNECTED, "ECU设备已连接");
        return true;
    }

    // 配置串口参数
    if (!configureSerialPort()) {
        QString errorMsg = "串口配置失败: " + m_config.serialConfig.port;
        qDebug() << "[ECUDevice] " << errorMsg;
        setStatus(Core::StatusCode::ERROR_CONFIG, errorMsg);
        return false;
    }
    qDebug() << "[ECUDevice] 串口配置成功:"
             << "端口=" << m_serialPort->portName()
             << "波特率=" << m_serialPort->baudRate()
             << "数据位=" << m_serialPort->dataBits()
             << "停止位=" << m_serialPort->stopBits()
             << "校验位=" << m_serialPort->parity();

    // 检查串口是否存在
    bool portFound = false;
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    qDebug() << "[ECUDevice] 系统可用串口列表:";
    for (const QSerialPortInfo &info : ports) {
        qDebug() << "  - 端口名:" << info.portName()
                 << "描述:" << info.description()
                 << "制造商:" << info.manufacturer();
        if (info.portName() == m_serialPort->portName()) {
            portFound = true;
        }
    }

    if (!portFound) {
        QString errorMsg = QString("串口不存在: %1").arg(m_serialPort->portName());
        qDebug() << "[ECUDevice] " << errorMsg;
        setStatus(Core::StatusCode::ERROR_CONFIG, errorMsg);
        return false;
    }

    // 打开串口
    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        QString errorMsg = QString("ECU设备连接失败: %1 - %2").arg(m_config.serialConfig.port).arg(m_serialPort->errorString());
        qDebug() << "[ECUDevice] " << errorMsg;
        setStatus(Core::StatusCode::ERROR_CONNECTION, errorMsg);
        return false;
    }

    // 清空缓冲区
    m_buffer.clear();

    qDebug() << "ECU设备连接成功:" << m_config.instanceName
             << "串口:" << m_config.serialConfig.port;
    setStatus(Core::StatusCode::CONNECTED, "ECU设备已连接");
    return true;
}

bool ECUDevice::disconnectDevice()
{
    // 确保停止采集
    stopAcquisition();

    // 关闭串口
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
        qDebug() << "ECU设备已断开连接:" << m_config.instanceName;
    }

    setStatus(Core::StatusCode::DISCONNECTED, "ECU设备已断开连接");
    return true;
}

void ECUDevice::startAcquisition()
{
    qDebug() << "[ECUDevice] 开始采集数据，设备:" << getDeviceId()
             << "线程ID:" << QThread::currentThreadId()
             << "当前状态:" << Core::statusCodeToString(m_status);

    // 只有在已连接状态下才能开始采集
    if (m_status != Core::StatusCode::CONNECTED && m_status != Core::StatusCode::STOPPED) {
        qDebug() << "[ECUDevice] 设备未连接，尝试连接设备:" << getDeviceId();

        // 尝试连接设备
        if (!connectDevice()) {
            QString errorMsg = "无法开始采集：设备连接失败";
            qDebug() << "[ECUDevice] " << errorMsg;
            emit errorOccurred(getDeviceId(), errorMsg);
            return;
        }

        qDebug() << "[ECUDevice] 设备连接成功，新状态:" << Core::statusCodeToString(m_status);
    }

    QMutexLocker locker(&m_mutex);

    // 如果已经在采集，不要重复启动
    if (m_isAcquiring) {
        qDebug() << "[ECUDevice] 设备已经在采集数据，忽略重复启动:" << getDeviceId();
        return;
    }

    m_isAcquiring = true;

    // 清空缓冲区
    m_buffer.clear();
    qDebug() << "[ECUDevice] 缓冲区已清空";

    // 确保定时器在当前线程中启动
    qDebug() << "[ECUDevice] 启动定时器，间隔:" << m_timer->interval() << "毫秒";

    // 检查定时器是否有效
    if (!m_timer) {
        qDebug() << "[ECUDevice] 错误：定时器对象为空!";
        m_isAcquiring = false;
        setStatus(Core::StatusCode::ERROR_CONFIG, "定时器对象为空");
        emit errorOccurred(getDeviceId(), "定时器对象为空");
        return;
    }

    // 检查定时器是否已经在运行
    if (m_timer->isActive()) {
        qDebug() << "[ECUDevice] 定时器已经在运行，不需要重新启动";
    } else {
        QMetaObject::invokeMethod(m_timer, "start", Qt::QueuedConnection);
        qDebug() << "[ECUDevice] 定时器启动命令已发送";
    }

    setStatus(Core::StatusCode::ACQUIRING, "ECU设备正在采集数据");
    qDebug() << "[ECUDevice] ECU设备" << m_config.instanceName << "开始采集数据，状态已更新为:" << Core::statusCodeToString(m_status);

    // ECU设备会自动发送数据，不需要发送请求帧
    if (m_serialPort && m_serialPort->isOpen()) {
        qDebug() << "[ECUDevice] 串口已打开，等待设备自动发送数据...";
    } else {
        qDebug() << "[ECUDevice] 警告：串口未打开，无法接收数据";
    }
}

void ECUDevice::stopAcquisition()
{
    QMutexLocker locker(&m_mutex);

    // 如果不在采集状态，直接返回
    if (!m_isAcquiring) {
        return;
    }

    // 停止定时器
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }

    m_isAcquiring = false;
    setStatus(Core::StatusCode::STOPPED, "ECU设备已停止采集");
    qDebug() << "ECU设备" << m_config.instanceName << "停止采集数据";
}

QString ECUDevice::getDeviceId() const
{
    return m_config.instanceName;
}

Core::DeviceType ECUDevice::getDeviceType() const
{
    return Core::DeviceType::ECU;
}

void ECUDevice::readECUData()
{
    qDebug() << "[ECUDevice] 定时器触发读取ECU数据，设备:" << getDeviceId() << "线程ID:" << QThread::currentThreadId();

    // 如果串口未打开，尝试连接
    if (!m_serialPort || !m_serialPort->isOpen()) {
        qDebug() << "[ECUDevice] 串口未打开，尝试连接...";
        if (!connectDevice()) {
            qDebug() << "[ECUDevice] 无法读取ECU数据：设备未连接";
            return;
        }
        qDebug() << "[ECUDevice] 串口连接成功";
    }

    // 检查是否有数据可读
    qint64 bytesAvailable = m_serialPort->bytesAvailable();
    qDebug() << "[ECUDevice] 串口可读字节数:" << bytesAvailable;

    if (bytesAvailable > 0) {
        qDebug() << "[ECUDevice] 有数据可读，处理数据...";
        handleSerialData();
    } else {
        qDebug() << "[ECUDevice] 没有数据可读，等待设备自动发送数据...";
    }
}

void ECUDevice::handleSerialData()
{
    QMutexLocker locker(&m_mutex);

    // 读取所有可用数据
    QByteArray data = m_serialPort->readAll();
    if (data.isEmpty()) {
        return;
    }

    // 打印接收到的原始数据（十六进制格式）
    QString hexData;
    for (int i = 0; i < data.size(); ++i) {
        hexData += QString("%1 ").arg(static_cast<quint8>(data.at(i)), 2, 16, QChar('0'));
    }
    qDebug() << "[ECUDevice] 接收到数据:" << hexData << "大小:" << data.size() << "字节";

    // 添加到缓冲区
    m_buffer.append(data);
    qDebug() << "[ECUDevice] 当前缓冲区大小:" << m_buffer.size() << "字节";

    // 检查是否有完整的帧
    // 帧格式: 0x7F 0x7F [数据...] [校验和] 0x0D 0x0A
    // 帧总长度为17字节
    const QByteArray frameHeader = QByteArray("\x7F\x7F", 2);
    const QByteArray frameFooter = QByteArray("\x0D\x0A", 2);

    qDebug() << "[ECUDevice] 查找帧头(0x7F 0x7F)和帧尾(0x0D 0x0A)";

    // 处理接收到的数据
    while (m_buffer.size() >= 17) {  // 最小帧长度
        // 查找帧头 (0x7F 0x7F)
        int startPos = m_buffer.indexOf(frameHeader);

        if (startPos < 0) {
            // 没有找到帧头，清空缓冲区
            qDebug() << "[ECUDevice] 未找到帧头，清空缓冲区";
            m_buffer.clear();
            break;
        }

        // 移除帧头之前的数据
        if (startPos > 0) {
            qDebug() << "[ECUDevice] 移除帧头之前的" << startPos << "字节数据";
            m_buffer.remove(0, startPos);
        }

        // 检查是否有完整的帧
        if (m_buffer.size() < 17) {
            // 数据不足，等待更多数据
            qDebug() << "[ECUDevice] 数据不足，等待更多数据";
            break;
        }

        // 检查帧尾 (0x0D 0x0A)
        if (static_cast<unsigned char>(m_buffer.at(15)) == 0x0D &&
            static_cast<unsigned char>(m_buffer.at(16)) == 0x0A) {

            // 提取完整帧
            QByteArray frame = m_buffer.mid(0, 17);

            // 打印帧内容（十六进制格式）
            QString frameHex;
            for (int i = 0; i < frame.size(); ++i) {
                frameHex += QString("%1 ").arg(static_cast<quint8>(frame.at(i)), 2, 16, QChar('0'));
            }
            qDebug() << "[ECUDevice] 提取完整帧:" << frameHex << "大小:" << frame.size() << "字节";

            // 验证校验和
            if (validateChecksum(frame)) {
                qDebug() << "[ECUDevice] 校验和验证通过";

                // 解析帧数据
                ECUFrameData frameData;
                if (parseFrame(frame, frameData)) {
                    qDebug() << "[ECUDevice] 帧解析成功:";
                    qDebug() << "  - 发动机转速:" << frameData.engineSpeed << "rpm";
                    qDebug() << "  - 节气门开度:" << frameData.throttlePosition << "%";
                    qDebug() << "  - 缸温:" << frameData.cylinderTemp << "°C";
                    qDebug() << "  - 排温:" << frameData.exhaustTemp << "°C";
                    qDebug() << "  - 燃油压力:" << frameData.fuelPressure << "kPa";
                    qDebug() << "  - 转子温度:" << frameData.rotorTemp << "°C";
                    qDebug() << "  - 进气温度:" << frameData.intakeTemp << "°C";
                    qDebug() << "  - 进气压力:" << frameData.intakePressure;
                    qDebug() << "  - 供电电压:" << frameData.supplyVoltage << "V";

                    // 发送数据到通道
                    emitChannelData(frameData);
                } else {
                    qDebug() << "[ECUDevice] 帧解析失败";
                }
            } else {
                qDebug() << "[ECUDevice] 校验和验证失败，丢弃帧";
            }

            // 移除已处理的帧
            m_buffer.remove(0, 17);
            qDebug() << "[ECUDevice] 从缓冲区移除已处理帧，剩余字节:" << m_buffer.size();
        } else {
            // 帧尾不匹配，移除第一个字节并继续寻找帧头
            qDebug() << "[ECUDevice] 帧尾不匹配，移除第一个字节并继续寻找";
            m_buffer.remove(0, 1);
        }
    }

    // 如果缓冲区过大，清除旧数据
    if (m_buffer.size() > 1024) {
        m_buffer.clear();
        qDebug() << "[ECUDevice] 缓冲区过大，已清空:" << m_config.instanceName;
    }
}

bool ECUDevice::validateChecksum(const QByteArray &frame)
{
    if (frame.size() < 15) {
        qDebug() << "[ECUDevice] 帧长度不足，无法验证校验和";
        return false;
    }

    unsigned char calculatedChecksum = 0;
    for (int i = 0; i <= 13; ++i) {
        calculatedChecksum += static_cast<unsigned char>(frame.at(i));
    }

    unsigned char receivedChecksum = static_cast<unsigned char>(frame.at(14));

    bool isValid = (calculatedChecksum == receivedChecksum);
    qDebug() << "[ECUDevice] 校验和验证:"
             << "计算值=" << QString("0x%1").arg(calculatedChecksum, 2, 16, QChar('0'))
             << "接收值=" << QString("0x%1").arg(receivedChecksum, 2, 16, QChar('0'))
             << "结果=" << (isValid ? "通过" : "失败");

    return isValid;
}

bool ECUDevice::parseFrame(const QByteArray &frame, ECUFrameData &data)
{
    // 检查帧长度是否足够
    if (frame.size() < 17) {
        qDebug() << "[ECUDevice] ECU帧长度不足:" << frame.size() << "，需要至少17字节";
        return false;
    }

    // 打印帧的每个字节（十六进制格式）
    qDebug() << "[ECUDevice] 开始解析帧，帧长度:" << frame.size() << "字节";
    qDebug() << "[ECUDevice] 帧头(0-1):"
             << QString("0x%1 0x%2").arg(static_cast<quint8>(frame.at(0)), 2, 16, QChar('0'))
             .arg(static_cast<quint8>(frame.at(1)), 2, 16, QChar('0'));

    // 解析转速 (字节2-3) - 直接转为float
    quint8 byte2 = static_cast<quint8>(frame.at(2));
    quint8 byte3 = static_cast<quint8>(frame.at(3));
    quint16 rawEngineSpeed = byte2 | (static_cast<quint16>(byte3) << 8);
    data.engineSpeed = static_cast<float>(rawEngineSpeed);
    qDebug() << "[ECUDevice] 转速字节:" << QString("0x%1 0x%2").arg(byte2, 2, 16, QChar('0')).arg(byte3, 2, 16, QChar('0'));

    // 解析节气门开度 (字节4-5) - 原始值需除以10.0得到百分比
    quint8 byte4 = static_cast<quint8>(frame.at(4));
    quint8 byte5 = static_cast<quint8>(frame.at(5));
    quint16 rawThrottlePosition = byte4 | (static_cast<quint16>(byte5) << 8);
    data.throttlePosition = static_cast<float>(rawThrottlePosition) / 10.0f;
    qDebug() << "[ECUDevice] 节气门开度字节:" << QString("0x%1 0x%2").arg(byte4, 2, 16, QChar('0')).arg(byte5, 2, 16, QChar('0'));

    // 解析缸温 (字节6) - 原始值减去偏移量后转为float
    qint8 byte6 = static_cast<qint8>(frame.at(6));
    data.cylinderTemp = static_cast<float>(byte6 - 40);
    qDebug() << "[ECUDevice] 缸温字节:" << QString("0x%1").arg(static_cast<quint8>(byte6), 2, 16, QChar('0'));

    // 解析排温 (字节7) - 原始值乘以系数后转为float
    quint8 byte7 = static_cast<quint8>(frame.at(7));
    data.exhaustTemp = static_cast<float>(byte7 * 5.0 - 40.0);
    qDebug() << "[ECUDevice] 排温字节:" << QString("0x%1").arg(byte7, 2, 16, QChar('0'));

    // 解析燃油压力 (字节8-9) - 直接转为float
    quint8 byte8 = static_cast<quint8>(frame.at(8));
    quint8 byte9 = static_cast<quint8>(frame.at(9));
    quint16 rawFuelPressure = byte8 | (static_cast<quint16>(byte9) << 8);
    data.fuelPressure = static_cast<float>(rawFuelPressure);
    qDebug() << "[ECUDevice] 燃油压力字节:" << QString("0x%1 0x%2").arg(byte8, 2, 16, QChar('0')).arg(byte9, 2, 16, QChar('0'));

    // 解析转子温度 (字节10) - 原始值减去偏移量后转为float
    qint8 byte10 = static_cast<qint8>(frame.at(10));
    data.rotorTemp = static_cast<float>(byte10 - 40);
    qDebug() << "[ECUDevice] 转子温度字节:" << QString("0x%1").arg(static_cast<quint8>(byte10), 2, 16, QChar('0'));

    // 解析进气温度 (字节11) - 原始值减去偏移量后转为float
    qint8 byte11 = static_cast<qint8>(frame.at(11));
    data.intakeTemp = static_cast<float>(byte11 - 40);
    qDebug() << "[ECUDevice] 进气温度字节:" << QString("0x%1").arg(static_cast<quint8>(byte11), 2, 16, QChar('0'));

    // 解析进气压力 (字节12) - 直接转为float
    quint8 byte12 = static_cast<quint8>(frame.at(12));
    data.intakePressure = static_cast<float>(byte12);
    qDebug() << "[ECUDevice] 进气压力字节:" << QString("0x%1").arg(byte12, 2, 16, QChar('0'));

    // 解析供电电压 (字节13) - 原始值除以10.0得到电压
    quint8 byte13 = static_cast<quint8>(frame.at(13));
    data.supplyVoltage = static_cast<float>(byte13) / 10.0f;
    qDebug() << "[ECUDevice] 供电电压字节:" << QString("0x%1").arg(byte13, 2, 16, QChar('0'));

    // 检查校验和和帧尾
    qDebug() << "[ECUDevice] 校验和字节(14):" << QString("0x%1").arg(static_cast<quint8>(frame.at(14)), 2, 16, QChar('0'));
    qDebug() << "[ECUDevice] 帧尾(15-16):"
             << QString("0x%1 0x%2").arg(static_cast<quint8>(frame.at(15)), 2, 16, QChar('0'))
             .arg(static_cast<quint8>(frame.at(16)), 2, 16, QChar('0'));

    qDebug() << "[ECUDevice] 帧解析完成";
    return true;
}

void ECUDevice::emitChannelData(const ECUFrameData &data)
{
    // 获取当前时间戳
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "[ECUDevice] 发送通道数据，时间戳:" << timestamp;

    int channelCount = 0;

    // 发送各个通道的数据
    if (m_channelParams.contains("speed")) {
        double value = applyFilter(static_cast<double>(data.engineSpeed));
        qDebug() << "[ECUDevice] 发送通道数据 - speed:" << value;
        emit rawDataPointReady(getDeviceId(), "speed", value, timestamp);
        channelCount++;
    } else {
        qDebug() << "[ECUDevice] 通道映射中不存在 speed 通道";
    }

    if (m_channelParams.contains("throttle_position")) {
        // 节气门开度已经在解析时除以10转换为百分比
        double value = applyFilter(static_cast<double>(data.throttlePosition));
        qDebug() << "[ECUDevice] 发送通道数据 - throttle_position:" << value;
        emit rawDataPointReady(getDeviceId(), "throttle_position", value, timestamp);
        channelCount++;
    } else {
        qDebug() << "[ECUDevice] 通道映射中不存在 throttle_position 通道";
    }

    if (m_channelParams.contains("cylinder_temp")) {
        double value = applyFilter(static_cast<double>(data.cylinderTemp));
        qDebug() << "[ECUDevice] 发送通道数据 - cylinder_temp:" << value;
        emit rawDataPointReady(getDeviceId(), "cylinder_temp", value, timestamp);
        channelCount++;
    } else {
        qDebug() << "[ECUDevice] 通道映射中不存在 cylinder_temp 通道";
    }

    if (m_channelParams.contains("exhaust_temp")) {
        double value = applyFilter(static_cast<double>(data.exhaustTemp));
        qDebug() << "[ECUDevice] 发送通道数据 - exhaust_temp:" << value;
        emit rawDataPointReady(getDeviceId(), "exhaust_temp", value, timestamp);
        channelCount++;
    } else {
        qDebug() << "[ECUDevice] 通道映射中不存在 exhaust_temp 通道";
    }

    if (m_channelParams.contains("fuel_pressure")) {
        double value = applyFilter(static_cast<double>(data.fuelPressure));
        qDebug() << "[ECUDevice] 发送通道数据 - fuel_pressure:" << value;
        emit rawDataPointReady(getDeviceId(), "fuel_pressure", value, timestamp);
        channelCount++;
    } else {
        qDebug() << "[ECUDevice] 通道映射中不存在 fuel_pressure 通道";
    }

    if (m_channelParams.contains("rotor_temp")) {
        double value = applyFilter(static_cast<double>(data.rotorTemp));
        qDebug() << "[ECUDevice] 发送通道数据 - rotor_temp:" << value;
        emit rawDataPointReady(getDeviceId(), "rotor_temp", value, timestamp);
        channelCount++;
    } else {
        qDebug() << "[ECUDevice] 通道映射中不存在 rotor_temp 通道";
    }

    if (m_channelParams.contains("intake_temp")) {
        double value = applyFilter(static_cast<double>(data.intakeTemp));
        qDebug() << "[ECUDevice] 发送通道数据 - intake_temp:" << value;
        emit rawDataPointReady(getDeviceId(), "intake_temp", value, timestamp);
        channelCount++;
    } else {
        qDebug() << "[ECUDevice] 通道映射中不存在 intake_temp 通道";
    }

    if (m_channelParams.contains("intake_pressure")) {
        double value = applyFilter(static_cast<double>(data.intakePressure));
        qDebug() << "[ECUDevice] 发送通道数据 - intake_pressure:" << value;
        emit rawDataPointReady(getDeviceId(), "intake_pressure", value, timestamp);
        channelCount++;
    } else {
        qDebug() << "[ECUDevice] 通道映射中不存在 intake_pressure 通道";
    }

    if (m_channelParams.contains("supply_voltage")) {
        double value = applyFilter(static_cast<double>(data.supplyVoltage));
        qDebug() << "[ECUDevice] 发送通道数据 - supply_voltage:" << value;
        emit rawDataPointReady(getDeviceId(), "supply_voltage", value, timestamp);
        channelCount++;
    } else {
        qDebug() << "[ECUDevice] 通道映射中不存在 supply_voltage 通道";
    }

    qDebug() << "[ECUDevice] 总共发送了" << channelCount << "个通道的数据";

    // 打印通道映射信息（仅在第一次发送数据时）
    static bool firstTime = true;
    if (firstTime) {
        qDebug() << "[ECUDevice] 通道映射信息:";
        for (auto it = m_channelParams.begin(); it != m_channelParams.end(); ++it) {
            qDebug() << "  - 通道名称:" << it.key()
                     << "增益:" << it.value().gain
                     << "偏移:" << it.value().offset;
        }
        firstTime = false;
    }
}

} // namespace Device
