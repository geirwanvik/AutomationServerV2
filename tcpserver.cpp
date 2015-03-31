#include "tcpserver.h"

TcpServer::TcpServer(QObject *parent) : QObject(parent)
{
    tcpServer = new QTcpServer(this);
    socket = new QTcpSocket(this);

    connect(tcpServer,SIGNAL(newConnection()),this,SLOT(newConnection()));

    if(!tcpServer->listen(QHostAddress::Any, 1234))
    {
        qDebug() << "Server could not start!";
    }
    else
    {
        qDebug() << "Server started";
    }
}

TcpServer::~TcpServer()
{

}

void TcpServer::newConnection()
{
    socket = tcpServer->nextPendingConnection();
    connect(socket,SIGNAL(readyRead()),this,SLOT(dataReceived()));
    connect(socket,SIGNAL(disconnected()),this,SLOT(disconnected()));
            qDebug() << "Client connected";
}

void TcpServer::disconnected()
{
    qDebug() << "Client disconnected";
}

void TcpServer::dataReceived()
{
    static quint16 nextBlockSize = 0;
    quint8 command = 0;

    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_3);
    if(nextBlockSize == 0)
    {
        if(socket->bytesAvailable() < sizeof(quint16))
        {
            return;
        }
        in >> nextBlockSize;
    }

    if(socket->bytesAvailable() < nextBlockSize)
    {
        return;
    }
    in >> command;


    processTelegram(command,in);
    nextBlockSize = 0;
}

void TcpServer::sendTelegram(quint8 command, QDataStream &out)
{
    QByteArray block;
    QDataStream telegram(&block,QIODevice::WriteOnly);
    telegram.setVersion(QDataStream::Qt_5_3);

    telegram << quint16(0);
    telegram << command;
    telegram << out;

    telegram.device()->seek(0);
    telegram << (quint16)(block.size() - sizeof(quint16));

    socket->write(block);
    socket->flush();
}

