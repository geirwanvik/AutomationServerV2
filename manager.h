#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include "telldusapi.h"
#include <QtSql>
#include "tcpserver.h"

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = 0);
    ~Manager();
    void databaseOpen();
    void databaseClose();
    void telldusInit();
    void syncCoreWithDatabase();


signals:
    void logEvent(QString event);

public slots:

private slots:
    void eventLogger(QString event);
    void deviceEvent(int eventId, int eventCommand, int eventValue, QString deviceType);
    void processTelegram(quint8 command, QDataStream &in);

private:
    TelldusApi *telldusApi;
    QSqlDatabase automationDatabase;
    TcpServer *tcpServer;
    bool lookingForDevices;
    bool streamingEvents;
};

#endif // MANAGER_H
