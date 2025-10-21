#ifndef PROTOCOLSENDER_H
#define PROTOCOLSENDER_H

#include <QObject>
#include <QByteArray>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

// 协议命令定义
enum ProtocolCommand {
    CMD_DEVICE_CONTROL = 0x03,  // 设备控制命令
    CMD_BLINK_EVENT = 0x0C      // 眨眼事件上报
};

// 设备控制状态定义
enum DeviceControlState {
    STATE_IDLE = 0x00,      // 空闲
    STATE_TREATMENT = 0x01, // 治疗中(开始)
    STATE_PAUSE = 0x02,     // 暂停
    STATE_RECOVERY = 0x03   // 恢复
};

class ProtocolSender : public QObject
{
    Q_OBJECT
public:
    explicit ProtocolSender(QObject* parent = nullptr);
    ~ProtocolSender();

    // 初始化TCP服务器
    bool initTcpServer(quint16 port);

    // 发送设备控制命令(新增)
    bool sendDeviceControl(quint8 state);

    // 发送开始命令(便捷方法)
    bool sendStart();

    // 发送眨眼事件
    bool sendBlinkEvent(quint8 channel, double intensity, double threshold_ratio);

    // 断开连接
    void disconnect();

    // 检查是否已连接
    bool isConnected() const { return m_isConnected; }

signals:
    void sendSuccess();
    void sendFailed(const QString& error);
    void connected();
    void disconnected();

private slots:
    // 新连接处理
    void onNewConnection();

private:
    // 构建协议帧
    QByteArray buildFrame(quint8 command, const QByteArray& data);

    // 计算校验和
    quint8 calculateChecksum(const QByteArray& data);

    // 发送数据
    bool sendData(const QByteArray& data);

    // TCP服务器对象
    QTcpServer* m_tcpServer;

    // 连接的客户端列表
    QList<QTcpSocket*> m_clients;

    // 是否已启动
    bool m_isConnected;
};

#endif // PROTOCOLSENDER_H