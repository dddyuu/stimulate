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
    // ����֮ǰ������
    disconnect();

    m_tcpServer = new QTcpServer(this);

    // �����������ź�
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

        // ������
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

        // ÿ�� IP ������һ�����ӣ����裩
        for (QTcpSocket* s : qAsConst(m_clients)) {
            if (s->peerAddress() == peer) {
                qDebug() << "Closing previous connection from" << peer.toString();
                s->disconnectFromHost();
            }
        }

        // TCP ѡ����� KeepAlive�����ӳ�
        socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

        // ��ȡ����־
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            const QByteArray bytes = socket->readAll();
            qDebug() << "Rx from" << socket->peerAddress().toString()
                << ":" << bytes.toHex(' ').toUpper();
            // ���Э����������/������������Իظ�һ֡ ACK/HELLO
            // ���磺socket->write(buildFrame(CMD_HELLO, QByteArray()));
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

        // ��ѡ����ʼ�ʺ�/���֣���������豸Э����Ҫ��
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

    // �����豸��������
    QByteArray data;

    // ״̬ (1�ֽ�)
    data.append(static_cast<char>(state));

    // ��������Э��֡
    QByteArray frame = buildFrame(CMD_DEVICE_CONTROL, data);

    // �������ݵ��������ӵĿͻ���
    bool result = sendData(frame);

    if (result) {
        QString stateStr;
        switch (state) {
        case STATE_IDLE: stateStr = "Free"; break;
        case STATE_TREATMENT: stateStr = "Start treatment"; break;
        case STATE_PAUSE: stateStr = "��ͣ"; break;
        case STATE_RECOVERY: stateStr = "�ָ�"; break;
        default: stateStr = "δ֪"; break;
        }
        qDebug() << "�豸���������ѷ��� - ״̬:" << stateStr << "(" << QString("0x%1").arg(state, 2, 16, QChar('0')).toUpper() << ")";
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

    // ����գ���¼�����
    QByteArray data;

    // ͨ�� (1�ֽ�)
    data.append(static_cast<char>(channel));

    // ǿ�� (2�ֽڣ��Ŵ�100����תΪ����)
    quint16 intensityInt = static_cast<quint16>(intensity * 100);
    data.append(static_cast<char>(intensityInt >> 8));   // ���ֽ�
    data.append(static_cast<char>(intensityInt & 0xFF)); // ���ֽ�

    // ��ֵ�� (2�ֽڣ��Ŵ�100����תΪ����)
    quint16 thresholdInt = static_cast<quint16>(threshold_ratio * 100);
    data.append(static_cast<char>(thresholdInt >> 8));   // ���ֽ�
    data.append(static_cast<char>(thresholdInt & 0xFF)); // ���ֽ�

    // ʱ��� (4�ֽڣ�����)
    quint32 timestamp = static_cast<quint32>(QDateTime::currentMSecsSinceEpoch() % 0xFFFFFFFF);
    data.append(static_cast<char>((timestamp >> 24) & 0xFF));
    data.append(static_cast<char>((timestamp >> 16) & 0xFF));
    data.append(static_cast<char>((timestamp >> 8) & 0xFF));
    data.append(static_cast<char>(timestamp & 0xFF));

    // ��������Э��֡
    QByteArray frame = buildFrame(CMD_BLINK_EVENT, data);

    // �������ݵ��������ӵĿͻ���
    return sendData(frame);
}

QByteArray ProtocolSender::buildFrame(quint8 command, const QByteArray& data)
{
    QByteArray frame;

    // ֡ͷ (1�ֽ�)
    frame.append(static_cast<char>(0xF1));

    // ֡�� = ֡ͷ(1) + ֡��(1) + ����(1) + ����(N) + У��(1)
    quint8 frameLength = 4 + data.size();
    frame.append(static_cast<char>(frameLength));

    // ���� (1�ֽ�)
    frame.append(static_cast<char>(command));

    // �����ֶ�
    frame.append(data);

    // У��� (1�ֽ�) - ��֡ͷ�����ݵ������ֽ����
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