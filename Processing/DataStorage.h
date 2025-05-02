#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QMap>
#include <QVector>
#include "../Core/DataTypes.h"
#include <QThread>

namespace Processing {

/**
 * @brief 数据存储类
 * 负责将采集的数据保存到文件
 */
class DataStorage : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DataStorage(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~DataStorage();

    /**
     * @brief 开始数据存储
     * @param startTimestamp 采集开始的时间戳（毫秒）
     * @return 是否成功开始存储
     */
    bool startStorage(qint64 startTimestamp);

    /**
     * @brief 停止数据存储
     * 关闭文件并完成存储
     */
    void stopStorage();

    /**
     * @brief 是否正在存储
     * @return 是否正在存储
     */
    bool isStoraging() const;

    /**
     * @brief 获取当前存储文件路径
     * @return 当前存储文件路径
     */
    QString getCurrentFilePath() const;

    /**
     * @brief 设置存储目录
     * @param dirPath 存储目录路径
     */
    void setStorageDirectory(const QString& dirPath);

public slots:
    /**
     * @brief 处理同步数据帧
     * @param frame 同步数据帧
     */
    void onSyncFrameReady(Core::SynchronizedDataFrame frame);

    /**
     * @brief 处理处理后数据点
     * @param channelId 通道ID
     * @param dataPoint 处理后的数据点
     */
    void onProcessedDataPointReady(QString channelId, Core::ProcessedDataPoint dataPoint);

signals:
    /**
     * @brief 存储状态变化信号
     * @param isStoraging 是否正在存储
     * @param filePath 存储文件路径
     */
    void storageStatusChanged(bool isStoraging, QString filePath);

    /**
     * @brief 存储错误信号
     * @param errorMsg 错误消息
     */
    void storageError(QString errorMsg);

private:
    /**
     * @brief 创建存储文件
     * @param startTimestamp 采集开始的时间戳（毫秒）
     * @return 是否成功创建文件
     */
    bool createStorageFile(qint64 startTimestamp);

    /**
     * @brief 写入文件头
     * @param channelIds 通道ID列表
     * @return 是否成功写入
     */
    bool writeHeader(const QStringList& channelIds);

    /**
     * @brief 写入数据行
     * @param frame 同步数据帧
     * @return 是否成功写入
     */
    bool writeDataRow(const Core::SynchronizedDataFrame& frame);

    /**
     * @brief 确保存储目录存在
     * @return 是否成功确保目录存在
     */
    bool ensureDirectoryExists();

private:
    QFile m_file;                        // 存储文件
    QTextStream m_stream;                // 文本流
    QString m_storageDirectory;          // 存储目录
    QString m_currentFilePath;           // 当前存储文件路径
    bool m_isStoraging;                  // 是否正在存储
    QStringList m_channelIds;            // 通道ID列表
    mutable QMutex m_mutex;              // 互斥锁
    qint64 m_startTimestamp;             // 采集开始的时间戳（毫秒）
    QMap<QString, Core::ProcessedDataPoint> m_latestDataPoints; // 最新的处理后数据点
};

} // namespace Processing

#endif // DATASTORAGE_H
