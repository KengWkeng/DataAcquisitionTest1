#include "DAQDevice.h"
#include <cmath>

// 定义必要的常量，确保这些常量在Art_DAQ.h中未定义的情况下可用
#ifndef ArtDAQ_Val_Cfg_Default
#define ArtDAQ_Val_Cfg_Default  -1
#endif

#ifndef ArtDAQ_Val_Volts
#define ArtDAQ_Val_Volts     10348
#endif

#ifndef ArtDAQ_Val_Rising
#define ArtDAQ_Val_Rising    10280
#endif

#ifndef ArtDAQ_Val_ContSamps
#define ArtDAQ_Val_ContSamps 10123
#endif

#ifndef ArtDAQ_Val_Acquired_Into_Buffer
#define ArtDAQ_Val_Acquired_Into_Buffer 1
#endif

#ifndef ArtDAQ_Val_GroupByScanNumber
#define ArtDAQ_Val_GroupByScanNumber 1
#endif

namespace Device {

// 全局变量，供回调函数使用
DAQDevice* g_daqDevice = nullptr;

DAQDevice::DAQDevice(const Core::DAQDeviceConfig& config, QObject *parent)
    : AbstractDevice(parent)
    , m_config(config)
    , m_taskHandle(0)
    , isAcquiring(false)
    , m_filterEnabled(false)
    , m_cutoffFrequency(50.0)  // 默认截止频率设置为50Hz
    , m_filterOrder(128)       // 默认滤波器阶数
{
    // 设置全局指针
    g_daqDevice = this;

    // 初始化通道映射
    for (const auto& channel : m_config.channels) {
        m_channelNames[channel.channelId] = channel.channelName;
        m_channelParams[channel.channelId] = channel.channelParams;
    }

    // 初始化滤波器系数
    calculateFilterCoefficients();

    qDebug() << "[DAQDevice] 初始化完成，设备ID:" << getDeviceId()
             << "，通道数:" << m_config.channels.size()
             << "，采样率:" << m_config.sampleRate;
}

DAQDevice::~DAQDevice()
{
    qDebug() << "[DAQDevice] 开始执行析构函数，设备ID:" << m_config.deviceId;

    // 确保任务已停止并清理资源
    if (isAcquiring) {
        // 停止采集
        stopAcquisition();
    }

    // 断开设备连接并清理任务
    disconnectDevice();

    // 确保任务句柄已清理
    if (m_taskHandle != 0) {
        qDebug() << "[DAQDevice] 析构函数中发现未清理的任务句柄，执行清理";
        char errBuff[2048] = {'\0'};
        int32 error = ArtDAQ_StopTask(m_taskHandle);
        if (error < 0) {
            ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
            qDebug() << "[DAQDevice] 析构函数中停止任务失败:" << errBuff;
        }

        error = ArtDAQ_ClearTask(m_taskHandle);
        if (error < 0) {
            ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
            qDebug() << "[DAQDevice] 析构函数中清理任务失败:" << errBuff;
        }
        m_taskHandle = 0;
    }

    // 清除全局指针 - 必须在最后执行，确保回调函数不会再访问已释放的对象
    {
        QMutexLocker locker(&m_mutex);
        if (g_daqDevice == this) {
            g_daqDevice = nullptr;
            qDebug() << "[DAQDevice] 全局指针已清除";
        }
    }

    qDebug() << "[DAQDevice] 析构函数执行完成，设备ID:" << m_config.deviceId;
}

bool DAQDevice::connectDevice()
{
    QMutexLocker locker(&m_mutex);

    // 如果已经连接，直接返回成功
    if (m_status == Core::StatusCode::CONNECTED || m_status == Core::StatusCode::ACQUIRING) {
        return true;
    }

    setStatus(Core::StatusCode::CONNECTING, "正在连接DAQ设备...");

    // 初始化DAQ任务
    initializeDAQTask();

    // 如果任务句柄有效，表示连接成功
    if (m_taskHandle != 0) {
        setStatus(Core::StatusCode::CONNECTED, "DAQ设备已连接");
        return true;
    } else {
        setStatus(Core::StatusCode::ERROR_CONNECTION, "DAQ设备连接失败");
        return false;
    }
}

bool DAQDevice::disconnectDevice()
{
    QMutexLocker locker(&m_mutex);

    // 如果正在采集，先停止采集
    if (isAcquiring) {
        // 在互斥锁内部调用stopAcquisition可能导致死锁，所以先解锁
        locker.unlock();
        stopAcquisition();
        locker.relock();
    }

    // 清理任务
    if (m_taskHandle != 0) {
        // 先停止任务，再清理
        int32 stopError = ArtDAQ_StopTask(m_taskHandle);
        if (stopError < 0) {
            char errBuff[2048] = {'\0'};
            ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
            qDebug() << "[DAQDevice] 停止任务失败:" << errBuff;
            // 即使停止失败，也要尝试清理
        }

        int32 clearError = ArtDAQ_ClearTask(m_taskHandle);
        if (clearError < 0) {
            char errBuff[2048] = {'\0'};
            ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
            qDebug() << "[DAQDevice] 清理任务失败:" << errBuff;
        } else {
            qDebug() << "[DAQDevice] 任务已成功清理";
        }
        m_taskHandle = 0;
    }

    setStatus(Core::StatusCode::DISCONNECTED, "DAQ设备已断开连接");
    qDebug() << "[DAQDevice] 设备已断开连接:" << getDeviceId();
    return true;
}

void DAQDevice::startAcquisition()
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
    if (isAcquiring) {
        qDebug() << "设备已经在采集数据，忽略重复启动:" << getDeviceId();
        return;
    }

    // 启动任务
    int error = 0;
    ArtDAQErrChk(ArtDAQ_StartTask(m_taskHandle));

    isAcquiring = true;
    setStatus(Core::StatusCode::ACQUIRING, "DAQ设备正在采集数据");
    qDebug() << "DAQ设备" << getDeviceId() << "开始采集数据";
    return;

Error:
    char errBuff[2048] = {'\0'};
    ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
    QString errorMsg = QString("启动DAQ任务失败: %1").arg(errBuff);
    emit errorOccurred(getDeviceId(), errorMsg);
    qDebug() << errorMsg;

    // 清理任务
    if (m_taskHandle != 0) {
        ArtDAQ_ClearTask(m_taskHandle);
        m_taskHandle = 0;
    }

    setStatus(Core::StatusCode::ERROR_CONNECTION, "启动DAQ任务失败");
}

void DAQDevice::stopAcquisition()
{
    QMutexLocker locker(&m_mutex);

    // 如果没有在采集，直接返回
    if (!isAcquiring) {
        return;
    }

    // 先更新状态，防止回调函数继续处理数据
    isAcquiring = false;

    // 停止任务
    if (m_taskHandle != 0) {
        char errBuff[2048] = {'\0'};
        int32 error = ArtDAQ_StopTask(m_taskHandle);
        if (error < 0) {
            ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
            qDebug() << "[DAQDevice] 停止任务失败:" << errBuff;

            // 即使停止失败，也要尝试清理任务
            error = ArtDAQ_ClearTask(m_taskHandle);
            if (error < 0) {
                ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
                qDebug() << "[DAQDevice] 清理任务失败:" << errBuff;
            } else {
                qDebug() << "[DAQDevice] 任务已成功清理";
                m_taskHandle = 0;
            }
        } else {
            qDebug() << "[DAQDevice] 任务已成功停止";

            // 停止成功后，清理任务
            error = ArtDAQ_ClearTask(m_taskHandle);
            if (error < 0) {
                ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
                qDebug() << "[DAQDevice] 清理任务失败:" << errBuff;
            } else {
                qDebug() << "[DAQDevice] 任务已成功清理";
                m_taskHandle = 0;
            }
        }
    }

    setStatus(Core::StatusCode::STOPPED, "DAQ设备已停止采集");
    qDebug() << "[DAQDevice] 设备" << getDeviceId() << "停止采集数据";
}

QString DAQDevice::getDeviceId() const
{
    return m_config.deviceId;
}

Core::DeviceType DAQDevice::getDeviceType() const
{
    return Core::DeviceType::DAQ;
}

void DAQDevice::setFilterEnabled(bool enabled)
{
    m_filterEnabled = enabled;
    qDebug() << "[DAQDevice] 滤波器状态已设置为:" << (enabled ? "启用" : "禁用");
}

void DAQDevice::setCutoffFrequency(double frequency)
{
    // 防止设置无效的截止频率
    if (frequency <= 0 || frequency >= m_config.sampleRate / 2.0) {
        qDebug() << "[DAQDevice] 无效的截止频率:" << frequency
                 << "必须在(0," << m_config.sampleRate/2.0 << ")范围内";
        return;
    }

    m_cutoffFrequency = frequency;
    qDebug() << "[DAQDevice] 截止频率已设置为:" << frequency << "Hz";

    // 重新计算滤波器系数
    calculateFilterCoefficients();
}

bool DAQDevice::isFilterEnabled() const
{
    return m_filterEnabled;
}

double DAQDevice::getCutoffFrequency() const
{
    return m_cutoffFrequency;
}

void DAQDevice::initializeDAQTask()
{
    // 如果任务已存在，先清理
    if (m_taskHandle != 0) {
        int32 error = ArtDAQ_ClearTask(m_taskHandle);
        if (error < 0) {
            char errBuff[2048] = {'\0'};
            ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
            qDebug() << "[DAQDevice] 清理现有任务失败:" << errBuff;
        }
        m_taskHandle = 0;
    }

    int error = 0;
    char errBuff[2048] = {'\0'};

    // 获取设备通道字符串
    QString deviceChannelStr = getDeviceChannelString();
    qDebug() << "[DAQDevice] 使用设备通道字符串: " << deviceChannelStr;

    // 初始化滤波缓冲区
    int numChannels = m_config.channels.size();
    m_filterBuffers.resize(numChannels);
    for (int ch = 0; ch < numChannels; ++ch) {
        m_filterBuffers[ch].resize(m_filterOrder + 1);
        m_filterBuffers[ch].fill(0.0); // 初始化为0
    }

    // 设置采样时钟参数 - 在使用ArtDAQErrChk之前定义所有变量
    int samplesPerChannel = 1000; // 每个通道的样本数
    int bufferSize = numChannels * samplesPerChannel;

    // 创建任务
    ArtDAQErrChk(ArtDAQ_CreateTask("", &m_taskHandle));

    // 创建模拟输入通道
    ArtDAQErrChk(ArtDAQ_CreateAIVoltageChan(m_taskHandle, deviceChannelStr.toStdString().c_str(), "",
                                      ArtDAQ_Val_Cfg_Default, -10.0, 10.0, ArtDAQ_Val_Volts, NULL));

    // 设置采样时钟
    ArtDAQErrChk(ArtDAQ_CfgSampClkTiming(m_taskHandle, "", m_config.sampleRate, ArtDAQ_Val_Rising,
                                   ArtDAQ_Val_ContSamps, samplesPerChannel));

    // 注册回调函数 - 每读取samplesPerChannel个样本触发一次回调
    ArtDAQErrChk(ArtDAQ_RegisterEveryNSamplesEvent(m_taskHandle, ArtDAQ_Val_Acquired_Into_Buffer,
                                            samplesPerChannel, 0, EveryNCallbackDAQ, NULL));

    // 注册完成事件回调
    ArtDAQErrChk(ArtDAQ_RegisterDoneEvent(m_taskHandle, 0, DoneCallbackDAQ, NULL));

    qDebug() << "[DAQDevice] DAQ任务初始化成功，设备ID:" << getDeviceId()
             << "，通道数:" << numChannels
             << "，采样率:" << m_config.sampleRate
             << "，缓冲区大小:" << bufferSize << "样本";

    return;

Error:
    ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
    QString errorMsg = QString("初始化DAQ任务失败: %1").arg(errBuff);
    emit errorOccurred(getDeviceId(), errorMsg);
    qDebug() << errorMsg;

    // 清理任务
    if (m_taskHandle != 0) {
        ArtDAQ_ClearTask(m_taskHandle);
        m_taskHandle = 0;
    }
}

void DAQDevice::processData(float64 *data, int32 read)
{
    // 检查数据有效性和设备状态
    if (read <= 0 || !isAcquiring || !data) {
        return;
    }

    try {
        // 获取当前时间戳
        qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

        // 获取通道数
        int numChannels = m_config.channels.size();
        if (numChannels <= 0) {
            qDebug() << "[DAQDevice] 无有效通道配置";
            return;
        }

        // 限制处理的数据量，避免一次处理过多数据导致UI卡顿
        // 如果数据量过大，只处理最新的部分数据
        const int maxProcessSamples = 100; // 减小处理样本数，降低内存占用
        int startSample = (read > maxProcessSamples) ? (read - maxProcessSamples) : 0;

        static int logCounter = 0;
        if (read > maxProcessSamples && ++logCounter % 100 == 0) {
            qDebug() << "[DAQDevice] 数据量过大，只处理最新的" << maxProcessSamples << "个样本，总样本数:" << read;
        }

        // 每个通道的数据是交错存储的，需要解交错
        for (int i = startSample; i < read; ++i) {
            for (int ch = 0; ch < numChannels; ++ch) {
                // 确保索引在有效范围内
                int dataIndex = i * numChannels + ch;
                if (dataIndex >= 0 && dataIndex < read * numChannels) {
                    double rawValue = data[dataIndex];

                    // 如果滤波器启用，应用滤波
                    if (m_filterEnabled) {
                        rawValue = applyFilter(rawValue, ch);
                    }

                    // 获取通道ID
                    if (ch < m_config.channels.size()) {
                        int channelId = m_config.channels[ch].channelId;

                        // 发送原始数据点 - 每个通道只发送最新的数据点，减少数据量
                        if (i == read - 1) {
                            emit rawDataPointReady(getDeviceId(), QString::number(channelId), rawValue, timestamp);
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        qDebug() << "[DAQDevice] 处理数据异常:" << e.what();
    }
    catch (...) {
        qDebug() << "[DAQDevice] 处理数据未知异常";
    }
}

void DAQDevice::calculateFilterCoefficients()
{
    // 重置滤波器系数
    m_filterCoefficients.resize(m_filterOrder + 1);

    // 归一化截止频率
    double normalizedCutoff = m_cutoffFrequency / m_config.sampleRate;

    // 限制截止频率在合理范围内
    if (normalizedCutoff > 0.45) {
        normalizedCutoff = 0.45; // 防止截止频率过高
        m_cutoffFrequency = normalizedCutoff * m_config.sampleRate;
        qDebug() << "[DAQDevice] 截止频率过高，已调整为:" << m_cutoffFrequency << "Hz";
    }

    // 计算滤波器系数
    double sum = 0.0;
    for (int i = 0; i <= m_filterOrder; i++) {
        // 计算滤波器系数
        double coef;
        if (i == m_filterOrder / 2) {
            // 中心点
            coef = 2.0 * normalizedCutoff;
        } else {
            // 其他点
            double x = 2.0 * M_PI * normalizedCutoff * (i - m_filterOrder / 2.0);
            coef = sin(x) / x;
        }

        // 应用Hamming窗函数 - 比汉宁窗有更好的侧带抑制
        // Hamming窗函数: 0.54 - 0.46 * cos(2πn/N)
        double hammingWindow = 0.54 - 0.46 * cos(2.0 * M_PI * i / m_filterOrder);
        m_filterCoefficients[i] = coef * hammingWindow;
        sum += m_filterCoefficients[i];
    }

    // 归一化系数，确保增益为1
    for (int i = 0; i <= m_filterOrder; i++) {
        m_filterCoefficients[i] /= sum;
    }

    qDebug() << "[DAQDevice] 已计算" << (m_filterOrder + 1) << "阶FIR滤波器系数，使用Hamming窗函数";
    qDebug() << "[DAQDevice] 截止频率:" << m_cutoffFrequency << "Hz，采样率:" << m_config.sampleRate << "Hz";
}

double DAQDevice::applyFilter(double sample, int channelIndex)
{
    // 检查通道索引是否有效
    if (channelIndex < 0 || channelIndex >= m_filterBuffers.size()) {
        return sample; // 如果无效，返回原始样本
    }

    // 确保缓冲区大小正确
    if (m_filterBuffers[channelIndex].size() != m_filterOrder + 1) {
        m_filterBuffers[channelIndex].resize(m_filterOrder + 1);
        m_filterBuffers[channelIndex].fill(sample); // 初始化为当前样本值，减少初始瞬态
    }

    // 移动缓冲区数据 - 使用更高效的方式
    memmove(&m_filterBuffers[channelIndex][1], &m_filterBuffers[channelIndex][0], m_filterOrder * sizeof(double));

    // 将新样本添加到缓冲区开头
    m_filterBuffers[channelIndex][0] = sample;

    // 应用FIR滤波器 - 使用向量化计算提高效率
    double result = 0.0;

    // 将卷积计算分成多个块以提高缓存命中率
    const int blockSize = 16; // 适合现代CPU缓存线的块大小

    for (int i = 0; i <= m_filterOrder; i += blockSize) {
        double blockSum = 0.0;
        int blockEnd = qMin(i + blockSize, m_filterOrder + 1);

        for (int j = i; j < blockEnd; j++) {
            blockSum += m_filterBuffers[channelIndex][j] * m_filterCoefficients[j];
        }

        result += blockSum;
    }

    return result;
}

QString DAQDevice::getDeviceChannelString() const
{
    QStringList channelList;

    for (const auto& channel : m_config.channels) {
        channelList.append(QString("%1/ai%2").arg(m_config.deviceId).arg(channel.channelId));
    }

    QString result = channelList.join(",");
    qDebug() << "生成设备通道字符串:" << result;
    return result;
}

// 全局回调函数实现
int32 ART_CALLBACK EveryNCallbackDAQ(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
{
    int32 error = 0;
    char errBuff[2048] = {'\0'};
    int32 read = 0;
    float64 *data = nullptr;

    // 检查全局指针是否有效
    if (!g_daqDevice) {
        qDebug() << "[DAQCallback] 回调函数: g_daqDevice无效";
        // 如果全局指针无效，但任务句柄有效，则清理任务
        if (taskHandle != 0) {
            ArtDAQ_StopTask(taskHandle);
            ArtDAQ_ClearTask(taskHandle);
            qDebug() << "[DAQCallback] 已清理无主任务";
        }
        return -1;
    }

    // 使用互斥锁保护访问，但设置超时，避免长时间阻塞
    if (!g_daqDevice->m_mutex.tryLock(100)) { // 100ms超时
        qDebug() << "[DAQCallback] 无法获取互斥锁，跳过本次数据处理";
        return 0;
    }

    // 再次检查设备是否在采集状态
    if (!g_daqDevice->isAcquiring) {
        qDebug() << "[DAQCallback] 设备不在采集状态";
        g_daqDevice->m_mutex.unlock();
        return 0; // 返回0表示成功，但不处理数据
    }

    // 创建足够大的缓冲区来存储所有通道的数据
    int numChannels = g_daqDevice->m_config.channels.size();
    int bufferSize = nSamples * numChannels;

    try {
        data = new float64[bufferSize];

        // 读取数据
        error = ArtDAQ_ReadAnalogF64(taskHandle, nSamples, 10.0, ArtDAQ_Val_GroupByScanNumber,
                               data, bufferSize, &read, NULL);

        if (error < 0) {
            ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
            qDebug() << "[DAQCallback] 读取数据失败: " << errBuff;

            // 发生错误时，停止任务并清理资源
            g_daqDevice->isAcquiring = false;
            g_daqDevice->m_mutex.unlock();

            // 停止和清理任务
            ArtDAQ_StopTask(taskHandle);
            ArtDAQ_ClearTask(taskHandle);
            g_daqDevice->m_taskHandle = 0;

            emit g_daqDevice->errorOccurred(g_daqDevice->getDeviceId(), QString("读取数据失败: %1").arg(errBuff));

            delete[] data;
            return -1;
        } else if (read > 0) {
            // 只在调试时输出详细信息，避免日志过多
            static int counter = 0;
            if (++counter % 100 == 0) {
                qDebug() << "[DAQCallback] 成功读取" << read << "个样本，" << numChannels << "个通道";
            }

            // 解锁互斥锁，避免在处理数据时长时间持有锁
            g_daqDevice->m_mutex.unlock();

            // 处理数据
            g_daqDevice->processData(data, read);
        } else {
            qDebug() << "[DAQCallback] 未读取到数据";
            g_daqDevice->m_mutex.unlock();
        }

        // 释放内存
        delete[] data;
        data = nullptr;
    }
    catch (const std::exception& e) {
        qDebug() << "[DAQCallback] 异常:" << e.what();
        if (data) {
            delete[] data;
        }
        g_daqDevice->m_mutex.unlock();
        return -1;
    }
    catch (...) {
        qDebug() << "[DAQCallback] 未知异常";
        if (data) {
            delete[] data;
        }
        g_daqDevice->m_mutex.unlock();
        return -1;
    }

    return 0;
}

int32 ART_CALLBACK DoneCallbackDAQ(TaskHandle taskHandle, int32 status, void *callbackData)
{
    int32 error = 0;
    char errBuff[2048] = {'\0'};

    // 检查全局指针是否有效
    if (!g_daqDevice) {
        qDebug() << "[DAQCallback] DoneCallback: g_daqDevice无效";
        // 如果全局指针无效，但任务句柄有效，则清理任务
        if (taskHandle != 0) {
            ArtDAQ_ClearTask(taskHandle);
            qDebug() << "[DAQCallback] 已清理无主任务";
        }
        return 0;
    }

    // 使用互斥锁保护访问，但设置超时，避免长时间阻塞
    if (!g_daqDevice->m_mutex.tryLock(100)) { // 100ms超时
        qDebug() << "[DAQCallback] DoneCallback: 无法获取互斥锁";
        // 即使无法获取锁，也要尝试清理任务
        if (taskHandle != 0) {
            ArtDAQ_ClearTask(taskHandle);
            qDebug() << "[DAQCallback] 已清理任务（无锁）";
        }
        return 0;
    }

    // 检查是否由于错误停止
    if (status < 0) {
        ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
        qDebug() << "[DAQCallback] 任务异常终止: " << errBuff;

        // 更新设备状态
        g_daqDevice->isAcquiring = false;

        // 清理任务句柄
        if (taskHandle != 0 && taskHandle == g_daqDevice->m_taskHandle) {
            g_daqDevice->m_taskHandle = 0;
        }

        // 解锁后再发送信号，避免死锁
        g_daqDevice->m_mutex.unlock();

        emit g_daqDevice->deviceStatusChanged(g_daqDevice->getDeviceId(),
                                             Core::StatusCode::ERROR_CONNECTION,
                                             QString("任务异常终止: %1").arg(errBuff));
    } else {
        qDebug() << "[DAQCallback] 任务正常完成";

        // 如果任务正常完成，但设备仍处于采集状态，则需要清理资源
        if (g_daqDevice->isAcquiring) {
            qDebug() << "[DAQCallback] 任务完成但设备仍在采集状态，执行清理";
            g_daqDevice->isAcquiring = false;

            // 清理任务句柄
            if (taskHandle != 0 && taskHandle == g_daqDevice->m_taskHandle) {
                g_daqDevice->m_taskHandle = 0;
            }

            // 解锁后再发送信号，避免死锁
            g_daqDevice->m_mutex.unlock();

            emit g_daqDevice->deviceStatusChanged(g_daqDevice->getDeviceId(),
                                                 Core::StatusCode::STOPPED,
                                                 "DAQ任务已完成");
        } else {
            g_daqDevice->m_mutex.unlock();
        }
    }

    // 确保任务被清理
    if (taskHandle != 0) {
        ArtDAQ_ClearTask(taskHandle);
        qDebug() << "[DAQCallback] DoneCallback: 已清理任务";
    }

    return 0;
}

} // namespace Device
