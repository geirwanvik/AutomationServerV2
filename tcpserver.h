#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QtNetwork>

class TcpServer : public QObject
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = 0);
    ~TcpServer();

signals:
    void logEvent(QString msg);
    void processTelegram(quint8 command, QDataStream &in);


public slots:
    void sendTelegram(quint8 command, QDataStream &out);


private slots:
    void newConnection();
    void disconnected();
    void dataReceived();

private:
    QTcpServer *tcpServer;
    QTcpSocket *socket;
};

#endif // TCPSERVER_H
