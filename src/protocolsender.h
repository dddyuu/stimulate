#ifndef PROTOCOLSENDER_H
#define PROTOCOLSENDER_H

#include <QObject>
#include <QByteArray>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

// Э�������
enum ProtocolCommand {
    CMD_DEVICE_CONTROL = 0x03,  // �豸��������
    CMD_BLINK_EVENT = 0x0C      // գ���¼��ϱ�
};

// �豸����״̬����
enum DeviceControlState {
    STATE_IDLE = 0x00,      // ����
    STATE_TREATMENT = 0x01, // ������(��ʼ)
    STATE_PAUSE = 0x02,     // ��ͣ
    STATE_RECOVERY = 0x03   // �ָ�
};

class ProtocolSender : public QObject
{
    Q_OBJECT
public:
    explicit ProtocolSender(QObject* parent = nullptr);
    ~ProtocolSender();

    // ��ʼ��TCP������
    bool initTcpServer(quint16 port);

    // �����豸��������(����)
    bool sendDeviceControl(quint8 state);

    // ���Ϳ�ʼ����(��ݷ���)
    bool sendStart();

    // ����գ���¼�
    bool sendBlinkEvent(quint8 channel, double intensity, double threshold_ratio);

    // �Ͽ�����
    void disconnect();

    // ����Ƿ�������
    bool isConnected() const { return m_isConnected; }

signals:
    void sendSuccess();
    void sendFailed(const QString& error);
    void connected();
    void disconnected();

private slots:
    // �����Ӵ���
    void onNewConnection();

private:
    // ����Э��֡
    QByteArray buildFrame(quint8 command, const QByteArray& data);

    // ����У���
    quint8 calculateChecksum(const QByteArray& data);

    // ��������
    bool sendData(const QByteArray& data);

    // TCP����������
    QTcpServer* m_tcpServer;

    // ���ӵĿͻ����б�
    QList<QTcpSocket*> m_clients;

    // �Ƿ�������
    bool m_isConnected;
};

#endif // PROTOCOLSENDER_H