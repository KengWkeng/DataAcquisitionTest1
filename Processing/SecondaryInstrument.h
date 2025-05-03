#ifndef SECONDARYINSTRUMENT_H
#define SECONDARYINSTRUMENT_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QStack>
#include <QMutex>
#include <QDebug>
#include "../Core/Constants.h"
#include "../Core/DataTypes.h"

namespace Processing {

/**
 * @brief 二次计算仪器类
 * 负责基于主通道数据进行二次计算
 */
class SecondaryInstrument : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config 二次计算仪器配置
     * @param parent 父对象
     */
    explicit SecondaryInstrument(const Core::SecondaryInstrumentConfig& config, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~SecondaryInstrument();

    /**
     * @brief 计算二次仪器值
     * @param channelValues 输入通道值映射
     * @param timestamp 时间戳
     * @return 处理后的数据点
     */
    Core::ProcessedDataPoint calculate(const QMap<QString, double>& channelValues, qint64 timestamp);

    /**
     * @brief 获取最新的处理后数据点
     * @return 最新的处理后数据点
     */
    Core::ProcessedDataPoint getLatestProcessedDataPoint() const;

    /**
     * @brief 获取通道ID
     * @return 通道ID
     */
    QString getChannelId() const;

    /**
     * @brief 获取通道名称
     * @return 通道名称
     */
    QString getChannelName() const;

    /**
     * @brief 获取输入通道列表
     * @return 输入通道列表
     */
    QStringList getInputChannels() const;

    /**
     * @brief 获取公式
     * @return 公式字符串
     */
    QString getFormula() const;

    /**
     * @brief 获取通道状态
     * @return 通道状态
     */
    Core::StatusCode getStatus() const;

    /**
     * @brief 设置通道状态
     * @param status 通道状态
     * @param message 状态消息
     */
    void setStatus(Core::StatusCode status, const QString& message = QString());

signals:
    /**
     * @brief 通道状态变化信号
     * @param channelId 通道ID
     * @param status 状态码
     * @param message 状态消息
     */
    void channelStatusChanged(QString channelId, Core::StatusCode status, QString message);

    /**
     * @brief 错误发生信号
     * @param channelId 通道ID
     * @param errorMsg 错误消息
     */
    void errorOccurred(QString channelId, QString errorMsg);

private:
    /**
     * @brief 解析公式
     * @param formula 公式字符串
     * @return 是否成功解析
     */
    bool parseFormula(const QString& formula);

    /**
     * @brief 计算公式值
     * @param channelValues 输入通道值映射
     * @return 计算结果
     */
    double evaluateFormula(const QMap<QString, double>& channelValues);

    /**
     * @brief 检查输入通道是否都可用
     * @param channelValues 输入通道值映射
     * @return 是否所有输入通道都可用
     */
    bool checkInputChannelsAvailable(const QMap<QString, double>& channelValues);

    /**
     * @brief 获取操作符优先级
     * @param op 操作符
     * @return 优先级（数字越大优先级越高）
     */
    int getOperatorPrecedence(const QChar& op) const;

    /**
     * @brief 应用操作符
     * @param op 操作符
     * @param a 第一个操作数
     * @param b 第二个操作数
     * @return 计算结果
     */
    double applyOperator(const QChar& op, double a, double b) const;

private:
    Core::SecondaryInstrumentConfig m_config;    // 二次计算仪器配置
    Core::ProcessedDataPoint m_latestDataPoint;  // 最新的处理后数据点
    mutable QMutex m_mutex;                      // 互斥锁
    Core::StatusCode m_status;                   // 通道状态
    QString m_statusMessage;                     // 状态消息
    
    // 解析后的公式元素
    enum TokenType { NUMBER, VARIABLE, OPERATOR, LEFT_PAREN, RIGHT_PAREN };
    struct Token {
        TokenType type;
        QString value;
        double number;
    };
    QVector<Token> m_tokens;                     // 解析后的公式标记
};

} // namespace Processing

#endif // SECONDARYINSTRUMENT_H
