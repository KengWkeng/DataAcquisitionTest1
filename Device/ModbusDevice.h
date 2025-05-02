#ifndef MODBUSDEVICE_H
#define MODBUSDEVICE_H

#include "AbstractDevice.h"
#include <QTimer>
#include <QDateTime>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QModbusRtuSerialClient>
#include <QModbusDataUnit>
#include <QModbusReply>
#include <QMap>
#include <QMutex>

namespace Device {

/**
 * @brief Modbus设备类
 * 用于与Modbus RTU设备通信
 */
class ModbusDevice : public AbstractDevice
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config Modbus设备配置
     * @param parent 父对象
     */
    explicit ModbusDevice(const Core::ModbusDeviceConfig& config, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ModbusDevice() override;

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
     * @brief 初始化Modbus客户端
     * 在线程启动后创建QModbusRtuSerialClient对象
     */
    void initializeModbusClient();

    /**
     * @brief 读取Modbus数据
     * 定时器触发时读取所有从站的寄存器
     */
    void readModbusData();

    /**
     * @brief 处理Modbus响应
     * @param reply Modbus响应对象
     */
    void processModbusResponse(QModbusReply *reply);

private:
    /**
     * @brief 配置串口参数
     * @return 是否成功配置
     */
    bool configureSerialPort();

    /**
     * @brief 读取从站寄存器
     * @param slaveId 从站ID
     * @param functionCode 功能码
     * @param startAddress 起始地址
     * @param count 寄存器数量
     * @return Modbus响应对象
     */
    QModbusReply* readRegisters(int slaveId, int functionCode, int startAddress, int count);

    /**
     * @brief 获取寄存器通道名称
     * @param slaveId 从站ID
     * @param registerAddress 寄存器地址
     * @return 通道名称
     */
    QString getRegisterChannelName(int slaveId, int registerAddress) const;

    /**
     * @brief 获取寄存器通道参数
     * @param slaveId 从站ID
     * @param registerAddress 寄存器地址
     * @return 通道参数
     */
    Core::ChannelParams getRegisterChannelParams(int slaveId, int registerAddress) const;

private:
    Core::ModbusDeviceConfig m_config;                // 设备配置
    QModbusRtuSerialClient* m_modbusClient;           // Modbus客户端
    QTimer* m_timer;                                  // 定时器
    QMap<int, QMap<int, QString>> m_channelNames;     // 从站ID -> 寄存器地址 -> 通道名称
    QMap<int, QMap<int, Core::ChannelParams>> m_channelParams; // 从站ID -> 寄存器地址 -> 通道参数
    QMutex m_mutex;                                   // 互斥锁，用于保护数据访问
    bool m_isAcquiring;                               // 是否正在采集
};

} // namespace Device

#endif // MODBUSDEVICE_H
