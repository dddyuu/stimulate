#include "protocolsender.h"
#include <QDateTime>

ProtocolSender::ProtocolSender(QObject* parent)
    : QObject(parent)
    , m_tcpServer(nullptr)
    , m_isConnected(false)
{
}

ProtocolSender::~ProtocolSender()
{
    disconnect();
}

bool ProtocolSender::initTcpServer(quint16 port)
{
    // 清理之前的连接
    disconnect();

    m_tcpServer = new QTcpServer(this);

    // 连接新连接信号
    connect(m_tcpServer, &QTcpServer::newConnection, this, &ProtocolSender::onNewConnection);

    //if (m_tcpServer->listen(QHostAddress::Any, port)) {
    if (m_tcpServer->listen(QHostAddress(QStringLiteral("192.168.1.100")), port)) {
        m_isConnected = true;
        qDebug() << "TCP server started successfully on port:" << port;
        emit connected();
        return true;
    }
    else {
        qDebug() << "Failed to start TCP server:" << m_tcpServer->errorString();
        emit sendFailed(m_tcpServer->errorString());
        return false;
    }
}

//void ProtocolSender::onNewConnection()
//{
//    QTcpSocket* socket = m_tcpServer->nextPendingConnection();
//    m_clients.append(socket);
//
//    connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
//        m_clients.removeOne(socket);
//        socket->deleteLater();
//        qDebug() << "Client disconnected";
//        });
//
//    qDebug() << "New client connected:" << socket->peerAddress().toString();
//}
void ProtocolSender::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* socket = m_tcpServer->nextPendingConnection();
        const QHostAddress peer = socket->peerAddress();
        const quint16 peerPort = socket->peerPort();

        // 白名单
        static const QList<QHostAddress> allowedIPs = {
            QHostAddress(QStringLiteral("192.168.1.80")),
            //QHostAddress(QStringLiteral("127.0.0.1")),
            
        };
        static const QList<QPair<QHostAddress, int>> allowedSubnets = {
            // { QHostAddress(QStringLiteral("192.168.1.0")), 24 },
        };

        const auto isAllowed = [&]() -> bool {
            bool ok = false;
            const quint32 v4 = peer.toIPv4Address(&ok);
            const QHostAddress peerV4 = ok ? QHostAddress(v4) : peer;
            for (const auto& ip : allowedIPs) {
                if (peer == ip || peerV4 == ip) return true;
            }
            for (const auto& sn : allowedSubnets) {
                if (peer.isInSubnet(sn) || peerV4.isInSubnet(sn)) return true;
            }
            return false;
            }();

        if (!isAllowed) {
            qDebug() << "Rejecting client from" << peer.toString() << ":" << peerPort;
            socket->disconnectFromHost();
            socket->deleteLater();
            continue;
        }

        // 每个 IP 仅保留一个连接（如需）
        for (QTcpSocket* s : qAsConst(m_clients)) {
            if (s->peerAddress() == peer) {
                qDebug() << "Closing previous connection from" << peer.toString();
                s->disconnectFromHost();
            }
        }

        // TCP 选项：启用 KeepAlive，低延迟
        socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

        // 读取与日志
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            const QByteArray bytes = socket->readAll();
            qDebug() << "Rx from" << socket->peerAddress().toString()
                << ":" << bytes.toHex(' ').toUpper();
            // 如果协议期望握手/心跳，这里可以回复一帧 ACK/HELLO
            // 例如：socket->write(buildFrame(CMD_HELLO, QByteArray()));
            });

    /*    connect(socket, &QTcpSocket::errorOccurred, this, [socket](QAbstractSocket::SocketError err) {
            Q_UNUSED(err);
            qDebug() << "Socket error from" << socket->peerAddress().toString()
                << ":" << socket->errorString();
            });*/

        connect(socket, &QTcpSocket::stateChanged, this, [socket](QAbstractSocket::SocketState st) {
            qDebug() << "State changed for" << socket->peerAddress().toString() << ":" << st;
            });

        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_clients.removeOne(socket);
            qDebug() << "Client disconnected from" << socket->peerAddress().toString();
            socket->deleteLater();
            });

        m_clients.append(socket);

        qDebug() << "New client connected from"
            << peer.toString() << ":" << peerPort
            << "to local"
            << socket->localAddress().toString() << ":" << socket->localPort();

        // 可选：初始问候/握手（仅当你的设备协议需要）
        // socket->write(buildFrame(CMD_HELLO, QByteArray()));
        // socket->flush();
    }
}

bool ProtocolSender::sendDeviceControl(quint8 state)
{
    if (!m_isConnected || m_clients.isEmpty()) {
        qDebug() << "TCP server not started or no clients connected, cannot send data";
        emit sendFailed("TCP server not started or no clients connected");
        return false;
    }

    // 构建设备控制数据
    QByteArray data;

    // 状态 (1字节)
    data.append(static_cast<char>(state));

    // 构建完整协议帧
    QByteArray frame = buildFrame(CMD_DEVICE_CONTROL, data);

    // 发送数据到所有连接的客户端
    bool result = sendData(frame);

    if (result) {
        QString stateStr;
        switch (state) {
        case STATE_IDLE: stateStr = "Free"; break;
        case STATE_TREATMENT: stateStr = "Start treatment"; break;
        case STATE_PAUSE: stateStr = "暂停"; break;
        case STATE_RECOVERY: stateStr = "恢复"; break;
        default: stateStr = "未知"; break;
        }
        qDebug() << "设备控制命令已发送 - 状态:" << stateStr << "(" << QString("0x%1").arg(state, 2, 16, QChar('0')).toUpper() << ")";
    }

    return result;
}

bool ProtocolSender::sendStart()
{
    return sendDeviceControl(STATE_TREATMENT);
}

bool ProtocolSender::sendBlinkEvent(quint8 channel, double intensity, double threshold_ratio)
{
    if (!m_isConnected || m_clients.isEmpty()) {
        qDebug() << "TCP server not started or no clients connected, cannot send data";
        emit sendFailed("TCP server not started or no clients connected");
        return false;
    }

    // 构建眨眼事件数据
    QByteArray data;

    // 通道 (1字节)
    data.append(static_cast<char>(channel));

    // 强度 (2字节，放大100倍后转为整数)
    quint16 intensityInt = static_cast<quint16>(intensity * 100);
    data.append(static_cast<char>(intensityInt >> 8));   // 高字节
    data.append(static_cast<char>(intensityInt & 0xFF)); // 低字节

    // 阈值比 (2字节，放大100倍后转为整数)
    quint16 thresholdInt = static_cast<quint16>(threshold_ratio * 100);
    data.append(static_cast<char>(thresholdInt >> 8));   // 高字节
    data.append(static_cast<char>(thresholdInt & 0xFF)); // 低字节

    // 时间戳 (4字节，毫秒)
    quint32 timestamp = static_cast<quint32>(QDateTime::currentMSecsSinceEpoch() % 0xFFFFFFFF);
    data.append(static_cast<char>((timestamp >> 24) & 0xFF));
    data.append(static_cast<char>((timestamp >> 16) & 0xFF));
    data.append(static_cast<char>((timestamp >> 8) & 0xFF));
    data.append(static_cast<char>(timestamp & 0xFF));

    // 构建完整协议帧
    QByteArray frame = buildFrame(CMD_BLINK_EVENT, data);

    // 发送数据到所有连接的客户端
    return sendData(frame);
}

QByteArray ProtocolSender::buildFrame(quint8 command, const QByteArray& data)
{
    QByteArray frame;

    // 帧头 (1字节)
    frame.append(static_cast<char>(0xF1));

    // 帧长 = 帧头(1) + 帧长(1) + 命令(1) + 数据(N) + 校验(1)
    quint8 frameLength = 4 + data.size();
    frame.append(static_cast<char>(frameLength));

    // 命令 (1字节)
    frame.append(static_cast<char>(command));

    // 数据字段
    frame.append(data);

    // 校验和 (1字节) - 对帧头到数据的所有字节求和
    quint8 checksum = calculateChecksum(frame);
    frame.append(static_cast<char>(checksum));

    return frame;
}

quint8 ProtocolSender::calculateChecksum(const QByteArray& data)
{
    quint8 sum = 0;
    for (int i = 0; i < data.size(); i++) {
        sum += static_cast<quint8>(data[i]);
    }
    return sum;
}

bool ProtocolSender::sendData(const QByteArray& data)
{
    if (!m_tcpServer || !m_tcpServer->isListening() || m_clients.isEmpty()) {
        qDebug() << "TCP server not listening or no clients";
        emit sendFailed("TCP server not listening or no clients");
        return false;
    }

    bool success = false;
    for (QTcpSocket* socket : m_clients) {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            qint64 bytesWritten = socket->write(data);
            socket->flush();
            if (bytesWritten > 0) {
                success = true;
                qDebug() << "Data sent to client:" << socket->peerAddress().toString() << bytesWritten << "bytes";
            }
        }
    }

    if (success) {
        qDebug() << "Data (HEX):" << data.toHex(' ').toUpper();
        emit sendSuccess();
        return true;
    }
    else {
        qDebug() << "Failed to send data to any client";
        emit sendFailed("Failed to send data to any client");
        return false;
    }
}

void ProtocolSender::disconnect()
{
    if (m_tcpServer) {
        m_tcpServer->close();
        for (QTcpSocket* socket : m_clients) {
            socket->disconnectFromHost();
            socket->deleteLater();
        }
        m_clients.clear();
        m_tcpServer->deleteLater();
        m_tcpServer = nullptr;
        qDebug() << "TCP server stopped";
    }

    m_isConnected = false;
    emit disconnected();
}