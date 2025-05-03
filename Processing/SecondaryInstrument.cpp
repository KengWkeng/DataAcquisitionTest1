#include "SecondaryInstrument.h"
#include <QThread>
#include <QRegularExpression>

namespace Processing {

SecondaryInstrument::SecondaryInstrument(const Core::SecondaryInstrumentConfig& config, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_status(Core::StatusCode::OK)
{
    // 初始化最新数据点
    m_latestDataPoint.channelId = m_config.channelName;
    m_latestDataPoint.value = 0.0;
    m_latestDataPoint.timestamp = 0;
    m_latestDataPoint.status = Core::StatusCode::OK;
    m_latestDataPoint.unit = "";  // 二次仪器暂不设置单位

    // 解析公式
    if (!parseFormula(m_config.formula)) {
        setStatus(Core::StatusCode::ERROR_CONFIG, "公式解析失败: " + m_config.formula);
    }

    qDebug() << "创建二次计算仪器:" << m_config.channelName
             << "公式:" << m_config.formula
             << "输入通道数量:" << m_config.inputChannels.size()
             << "线程ID:" << QThread::currentThreadId();

    // 添加显示格式调试输出
    qDebug() << "二次计算仪器显示格式:" << m_config.channelName
             << "中文标签=" << m_config.displayFormat.labelInChinese
             << "采集类型=" << m_config.displayFormat.acquisitionType
             << "单位=" << m_config.displayFormat.unit
             << "分辨率=" << m_config.displayFormat.resolution
             << "范围=[" << m_config.displayFormat.minRange << "," << m_config.displayFormat.maxRange << "]";
}

SecondaryInstrument::~SecondaryInstrument()
{
    qDebug() << "销毁二次计算仪器:" << m_config.channelName
             << "线程ID:" << QThread::currentThreadId();
}

Core::ProcessedDataPoint SecondaryInstrument::calculate(const QMap<QString, double>& channelValues, qint64 timestamp)
{
    QMutexLocker locker(&m_mutex);

    // 检查输入通道是否都可用
    if (!checkInputChannelsAvailable(channelValues)) {
        // 如果有输入通道不可用，返回上一次的数据点，但更新时间戳
        m_latestDataPoint.timestamp = timestamp;
        return m_latestDataPoint;
    }

    // 计算公式值
    double result = evaluateFormula(channelValues);

    // 创建处理后的数据点
    Core::ProcessedDataPoint dataPoint;
    dataPoint.channelId = m_config.channelName;
    dataPoint.value = result;
    dataPoint.timestamp = timestamp;
    dataPoint.status = m_status;
    dataPoint.unit = m_latestDataPoint.unit;

    // 更新最新数据点
    m_latestDataPoint = dataPoint;

    // 记录处理信息
    qDebug() << "二次计算仪器处理数据:" << m_config.channelName
             << "结果:" << result
             << "线程ID:" << QThread::currentThreadId();

    return dataPoint;
}

Core::ProcessedDataPoint SecondaryInstrument::getLatestProcessedDataPoint() const
{
    QMutexLocker locker(&m_mutex);
    return m_latestDataPoint;
}

QString SecondaryInstrument::getChannelId() const
{
    return m_config.channelName;
}

QString SecondaryInstrument::getChannelName() const
{
    return m_config.channelName;
}

QStringList SecondaryInstrument::getInputChannels() const
{
    return m_config.inputChannels;
}

QString SecondaryInstrument::getFormula() const
{
    return m_config.formula;
}

Core::StatusCode SecondaryInstrument::getStatus() const
{
    QMutexLocker locker(&m_mutex);
    return m_status;
}

void SecondaryInstrument::setStatus(Core::StatusCode status, const QString& message)
{
    QMutexLocker locker(&m_mutex);

    if (m_status != status || m_statusMessage != message) {
        m_status = status;
        m_statusMessage = message;

        // 更新最新数据点的状态
        m_latestDataPoint.status = status;

        // 发送状态变化信号
        emit channelStatusChanged(m_config.channelName, m_status, m_statusMessage);

        // 记录状态变化
        qDebug() << "二次计算仪器" << m_config.channelName << "状态变为"
                 << Core::statusCodeToString(m_status) << ":" << m_statusMessage
                 << "线程ID:" << QThread::currentThreadId();
    }
}

bool SecondaryInstrument::parseFormula(const QString& formula)
{
    // 清空之前的标记
    m_tokens.clear();

    // 定义正则表达式模式
    QRegularExpression tokenRegex(
        "\\s*([0-9]+(\\.[0-9]+)?|[a-zA-Z_][a-zA-Z0-9_]*|\\+|\\-|\\*|\\/|\\(|\\))\\s*");

    // 解析公式
    int pos = 0;
    while (pos < formula.length()) {
        QRegularExpressionMatch match = tokenRegex.match(formula, pos);
        if (!match.hasMatch()) {
            qDebug() << "公式解析错误，无法识别的标记:" << formula.mid(pos);
            return false;
        }

        QString token = match.captured(1);
        pos = match.capturedEnd();

        // 判断标记类型
        if (token == "+" || token == "-" || token == "*" || token == "/") {
            // 操作符
            Token t;
            t.type = OPERATOR;
            t.value = token;
            m_tokens.append(t);
        } else if (token == "(") {
            // 左括号
            Token t;
            t.type = LEFT_PAREN;
            t.value = token;
            m_tokens.append(t);
        } else if (token == ")") {
            // 右括号
            Token t;
            t.type = RIGHT_PAREN;
            t.value = token;
            m_tokens.append(t);
        } else if (token[0].isDigit()) {
            // 数字
            Token t;
            t.type = NUMBER;
            t.value = token;
            t.number = token.toDouble();
            m_tokens.append(t);
        } else {
            // 变量（通道名）
            Token t;
            t.type = VARIABLE;
            t.value = token;
            m_tokens.append(t);

            // 检查变量是否在输入通道列表中
            if (!m_config.inputChannels.contains(token)) {
                qDebug() << "公式中的变量不在输入通道列表中:" << token;
                return false;
            }
        }
    }

    qDebug() << "公式解析成功:" << formula << "，标记数量:" << m_tokens.size();
    return true;
}

double SecondaryInstrument::evaluateFormula(const QMap<QString, double>& channelValues)
{
    // 使用调度场算法（Shunting Yard Algorithm）计算公式
    QStack<double> values;
    QStack<QChar> operators;

    for (const Token& token : m_tokens) {
        switch (token.type) {
        case NUMBER:
            values.push(token.number);
            break;
        case VARIABLE:
            values.push(channelValues.value(token.value, 0.0));
            break;
        case LEFT_PAREN:
            operators.push('(');
            break;
        case RIGHT_PAREN:
            // 处理右括号，计算括号内的所有操作
            while (!operators.isEmpty() && operators.top() != '(') {
                QChar op = operators.pop();
                double b = values.pop();
                double a = values.pop();
                values.push(applyOperator(op, a, b));
            }
            // 弹出左括号
            if (!operators.isEmpty() && operators.top() == '(') {
                operators.pop();
            }
            break;
        case OPERATOR:
            QChar currentOp = token.value[0];
            // 处理操作符优先级
            while (!operators.isEmpty() && operators.top() != '(' &&
                   getOperatorPrecedence(operators.top()) >= getOperatorPrecedence(currentOp)) {
                QChar op = operators.pop();
                double b = values.pop();
                double a = values.pop();
                values.push(applyOperator(op, a, b));
            }
            operators.push(currentOp);
            break;
        }
    }

    // 处理剩余的操作符
    while (!operators.isEmpty()) {
        QChar op = operators.pop();
        double b = values.pop();
        double a = values.pop();
        values.push(applyOperator(op, a, b));
    }

    // 结果应该在值栈的顶部
    if (values.isEmpty()) {
        qDebug() << "公式计算错误，结果栈为空";
        return 0.0;
    }

    return values.top();
}

bool SecondaryInstrument::checkInputChannelsAvailable(const QMap<QString, double>& channelValues)
{
    for (const QString& channel : m_config.inputChannels) {
        if (!channelValues.contains(channel)) {
            qDebug() << "输入通道不可用:" << channel;
            return false;
        }
    }
    return true;
}

int SecondaryInstrument::getOperatorPrecedence(const QChar& op) const
{
    if (op == '*' || op == '/') {
        return 2;
    } else if (op == '+' || op == '-') {
        return 1;
    }
    return 0;
}

double SecondaryInstrument::applyOperator(const QChar& op, double a, double b) const
{
    switch (op.toLatin1()) {
    case '+': return a + b;
    case '-': return a - b;
    case '*': return a * b;
    case '/':
        if (b == 0.0) {
            qDebug() << "除零错误";
            return 0.0;
        }
        return a / b;
    default:
        qDebug() << "未知操作符:" << op;
        return 0.0;
    }
}

} // namespace Processing
