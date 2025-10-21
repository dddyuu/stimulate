#include "tcpconnectwidget.h"
#include "ui_tcpconnectwidget.h"
#include "QHostAddress"
#include "QNetworkInterface"
TCPConnectWidget::TCPConnectWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TCPConnectWidget),status(false)
{
    ui->setupUi(this);
    ui->ip->setText(read_ip_address());
    ui->port->setText("5001");
}

TCPConnectWidget::~TCPConnectWidget()
{
    delete ui;
}
void TCPConnectWidget::on_listen_clicked()
{
    if(!status)
    {
        quint16 port=ui->port->text().toUInt();
        status=!status;
        ui->listen->setText("取消监听");
        emit start(port);
    }
    else
    {
        status=!status;
         ui->listen->setText("开始监听");
        emit close();
    }


}

QString TCPConnectWidget::read_ip_address()
{
    QString ip_address;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    for (int i = 0; i < ipAddressesList.size(); ++i)
    {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&  ipAddressesList.at(i).toIPv4Address())
        {
            ip_address = ipAddressesList.at(i).toString();
        }
    }
    if (ip_address.isEmpty())
        ip_address = QHostAddress(QHostAddress::LocalHost).toString();
    return ip_address;
}
