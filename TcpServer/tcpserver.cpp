#include "tcpserver.h"
#include "ui_tcpserver.h"
#include "mytcpserver.h"
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QHostAddress>

TcpServer::TcpServer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TcpServer)
{
    ui->setupUi(this);
    loadConfig();
    MyTcpServer::getInstance().listen(QHostAddress(m_strIP),m_usPort);
}

TcpServer::~TcpServer()
{
    delete ui;

}

void TcpServer::loadConfig()
{
    QFile file(":/server.config.txt");
    if (file.open(QIODeviceBase::ReadOnly))
    {
        QByteArray baDate=file.readAll();
        QString strData= baDate.toStdString().c_str();
        //qDebug()<< strData;
        file.close();

        strData.replace("\r\n"," ");
        // qDebug()<< strData;

        QStringList strList=strData.split(" ");
        // for(int i=0;i<strList.size();i++)
        // {
        //     qDebug()<<"--->"<<strList[i];
        // }
        m_strIP= strList.at(0);
        m_usPort=strList.at(1).toUShort();
        //qDebug()<<"IP:"<<m_strIP<<"Port:"<<m_usPort;

    }
    else
    {
        QMessageBox::critical(this, "opne confog","opne config false");
    }
}
