#ifndef ECUDEVICE_H
#define ECUDEVICE_H

#include "AbstractDevice.h"
#include <QTimer>
#include <QDateTime>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMap>
#include <QMutex>
#include <QByteArray>

namespace Device {

/**
 * @brief ECU帧数据结构体
 * 存储ECU设备发送的各种数据
 */
struct ECUFrameData {
    float engineSpeed;        // 发动机转速 (rpm)
    float throttlePosition;   // 节气门开度 (%)
    float cylinderTemp;       // 缸温 (°C)
    float exhaustTemp;        // 排温 (°C)
    float fuelPressure;       // 燃油压力 (kPa)
    float rotorTemp;          // 转子温度 (°C)
    float intakeTemp;         // 进气温度 (°C)
    float intakePressure;     // 进气压力
    float supplyVoltage;      // 供电电压 (V)
};

/**
 * @brief ECU设备类
 * 用于与ECU设备通信
 */
class ECUDevice : public AbstractDevice
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config ECU设备配置
     * @param parent 父对象
     */
    explicit ECUDevice(const Core::ECUDeviceConfig& config, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ECUDevice() override;

    /**
     * @brief 连接设备
     * @return 是否成功连接
     */
    bool connectDevice() override;

    /**
     * @brief 断开设备连接
     * @return 是否成功断开
     */
    bool disconnectDevice() override;

    /**
     * @brief 开始数据采集
     */
    void startAcquisition() override;

    /**
     * @brief 停止数据采集
     */
    void stopAcquisition() override;

    /**
     * @brief 获取设备ID
     * @return 设备ID
     */
    QString getDeviceId() const override;

    /**
     * @brief 获取设备类型
     * @return 设备类型
     */
    Core::DeviceType getDeviceType() const override;

private slots:
    /**
     * @brief 初始化串口
     * 在线程启动后调用，确保在正确的线程中创建QSerialPort
     */
    void initializeSerialPort();

    /**
     * @brief 读取ECU数据
     * 定时器触发时读取串口数据
     */
    void readECUData();

    /**
     * @brief 处理串口数据
     * 当串口有数据可读时调用
     */
    void handleSerialData();

private:
    /**
     * @brief 配置串口参数
     * @return 是否成功配置
     */
    bool configureSerialPort();

    /**
     * @brief 解析ECU帧数据
     * @param frame 接收到的数据帧
     * @param data 解析后的数据结构
     * @return 是否成功解析
     */
    bool parseFrame(const QByteArray &frame, ECUFrameData &data);

    /**
     * @brief 验证帧校验和
     * @param frame 接收到的数据帧
     * @return 校验和是否正确
     */
    bool validateChecksum(const QByteArray &frame);

    /**
     * @brief 发送数据到通道
     * @param data 解析后的ECU数据
     */
    void emitChannelData(const ECUFrameData &data);

private:
    Core::ECUDeviceConfig m_config;  // ECU设备配置
    QSerialPort* m_serialPort;       // 串口对象
    QTimer* m_timer;                 // 定时器
    QByteArray m_buffer;             // 数据缓冲区
    bool m_isAcquiring;              // 是否正在采集
    QMutex m_mutex;                  // 互斥锁
    QMap<QString, Core::ChannelParams> m_channelParams; // 通道参数映射
    qint64 m_lastDataTime;           // 最后接收数据的时间戳
};

} // namespace Device

#endif // ECUDEVICE_H
