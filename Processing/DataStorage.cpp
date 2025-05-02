#include "DataStorage.h"
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

namespace Processing {

DataStorage::DataStorage(QObject *parent)
    : QObject(parent)
    , m_isStoraging(false)
    , m_startTimestamp(0)
{
    // 默认存储目录为应用程序目录下的Data文件夹
    m_storageDirectory = QCoreApplication::applicationDirPath() + "/Data";
    
    qDebug() << "创建数据存储器，存储目录:" << m_storageDirectory
             << "线程ID:" << QThread::currentThreadId();
}

DataStorage::~DataStorage()
{
    // 停止存储
    stopStorage();
    
    qDebug() << "销毁数据存储器，线程ID:" << QThread::currentThreadId();
}

bool DataStorage::startStorage(qint64 startTimestamp)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_isStoraging) {
        qDebug() << "数据存储已经在进行中!";
        return false;
    }
    
    // 保存开始时间戳
    m_startTimestamp = startTimestamp;
    
    // 创建存储文件
    if (!createStorageFile(startTimestamp)) {
        return false;
    }
    
    m_isStoraging = true;
    
    // 发送存储状态变化信号
    emit storageStatusChanged(m_isStoraging, m_currentFilePath);
    
    qDebug() << "开始数据存储，文件:" << m_currentFilePath
             << "线程ID:" << QThread::currentThreadId();
    
    return true;
}

void DataStorage::stopStorage()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isStoraging) {
        return;
    }
    
    // 关闭文件
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
    
    m_isStoraging = false;
    
    // 发送存储状态变化信号
    emit storageStatusChanged(m_isStoraging, m_currentFilePath);
    
    qDebug() << "停止数据存储，文件:" << m_currentFilePath
             << "线程ID:" << QThread::currentThreadId();
}

bool DataStorage::isStoraging() const
{
    QMutexLocker locker(&m_mutex);
    return m_isStoraging;
}

QString DataStorage::getCurrentFilePath() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentFilePath;
}

void DataStorage::setStorageDirectory(const QString& dirPath)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_isStoraging) {
        qDebug() << "数据存储进行中，无法更改存储目录!";
        return;
    }
    
    m_storageDirectory = dirPath;
    
    qDebug() << "设置存储目录:" << m_storageDirectory;
}

void DataStorage::onSyncFrameReady(Core::SynchronizedDataFrame frame)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isStoraging || !m_file.isOpen()) {
        return;
    }
    
    // 写入数据行
    if (!writeDataRow(frame)) {
        qDebug() << "写入数据行失败!";
        emit storageError("写入数据行失败: " + m_file.errorString());
    }
}

void DataStorage::onProcessedDataPointReady(QString channelId, Core::ProcessedDataPoint dataPoint)
{
    QMutexLocker locker(&m_mutex);
    
    // 更新最新的处理后数据点
    m_latestDataPoints[channelId] = dataPoint;
    
    // 如果通道ID不在列表中，添加到列表
    if (!m_channelIds.contains(channelId)) {
        m_channelIds.append(channelId);
        
        // 如果正在存储，需要重写文件头
        if (m_isStoraging && m_file.isOpen()) {
            // 重置文件位置到开头
            m_file.seek(0);
            
            // 重写文件头
            if (!writeHeader(m_channelIds)) {
                qDebug() << "重写文件头失败!";
                emit storageError("重写文件头失败: " + m_file.errorString());
            }
        }
    }
}

bool DataStorage::createStorageFile(qint64 startTimestamp)
{
    // 确保存储目录存在
    if (!ensureDirectoryExists()) {
        return false;
    }
    
    // 根据开始时间戳创建文件名
    QString timeStr = QDateTime::fromMSecsSinceEpoch(startTimestamp).toString("yyyyMMdd_HHmmss");
    m_currentFilePath = m_storageDirectory + "/" + timeStr + ".csv";
    
    // 打开文件
    m_file.setFileName(m_currentFilePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法创建存储文件:" << m_currentFilePath << "错误:" << m_file.errorString();
        emit storageError("无法创建存储文件: " + m_file.errorString());
        return false;
    }
    
    // 设置文本流
    m_stream.setDevice(&m_file);
    
    // 写入文件头
    if (!writeHeader(m_channelIds)) {
        qDebug() << "写入文件头失败!";
        emit storageError("写入文件头失败: " + m_file.errorString());
        m_file.close();
        return false;
    }
    
    return true;
}

bool DataStorage::writeHeader(const QStringList& channelIds)
{
    // 写入文件头
    m_stream << "Timestamp,RelativeTime(s)";
    
    // 写入通道ID
    for (const QString& channelId : channelIds) {
        m_stream << "," << channelId;
    }
    
    m_stream << "\n";
    
    return m_stream.status() == QTextStream::Ok;
}

bool DataStorage::writeDataRow(const Core::SynchronizedDataFrame& frame)
{
    // 写入时间戳
    m_stream << frame.timestamp;
    
    // 写入相对时间（秒）
    double relativeTime = (frame.timestamp - m_startTimestamp) / 1000.0;
    m_stream << "," << QString::number(relativeTime, 'f', 3);
    
    // 写入各通道的值
    for (const QString& channelId : m_channelIds) {
        if (frame.channelData.contains(channelId)) {
            m_stream << "," << QString::number(frame.channelData[channelId].value, 'f', 6);
        } else {
            m_stream << ",";
        }
    }
    
    m_stream << "\n";
    
    // 定期刷新缓冲区
    static int rowCount = 0;
    if (++rowCount % 100 == 0) {
        m_stream.flush();
    }
    
    return m_stream.status() == QTextStream::Ok;
}

bool DataStorage::ensureDirectoryExists()
{
    QDir dir(m_storageDirectory);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "无法创建存储目录:" << m_storageDirectory;
            emit storageError("无法创建存储目录: " + m_storageDirectory);
            return false;
        }
    }
    
    return true;
}

} // namespace Processing
