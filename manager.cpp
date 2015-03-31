#include "manager.h"

Manager::Manager(QObject *parent) : QObject(parent)
{
    tcpServer = new TcpServer(this);

    connect(this,SIGNAL(logEvent(QString)),this,SLOT(eventLogger(QString)));

    databaseOpen();

    telldusInit();
    lookingForDevices = false;
    streamingEvents = false;

}

Manager::~Manager()
{

}

void Manager::databaseOpen()
{
    if (!QSqlDatabase::drivers().contains("QMYSQL"))
        emit logEvent(tr("Qt cannot access the MYSQL driver"));


    automationDatabase = QSqlDatabase::addDatabase("QMYSQL");
    automationDatabase.setHostName("10.0.0.100");
    automationDatabase.setPort(3306);
    automationDatabase.setDatabaseName("HomeAutomation");
    automationDatabase.setUserName("lenny");
    automationDatabase.setPassword("linux");
    bool ok = automationDatabase.open();

    if(ok)
    {
        emit logEvent(tr("Connection to database HomeAutomation open"));
    }
    else
    {
        emit logEvent(tr("Error opening the database HomeAutomation, sql error: %1")
                      .arg(automationDatabase.lastError().text()));
    }
}

void Manager::databaseClose()
{
    automationDatabase.close();
}

void Manager::deviceEvent(int eventId, int eventCommand, int eventValue, QString deviceType)
{
    qDebug() << "Event from id " << eventId << "command " << eventCommand << "value " << eventValue;
    bool unknownDevice = true;

    QSqlQuery qry(QSqlDatabase::database(automationDatabase.connectionName()));
    qry.prepare("SELECT id FROM commandDevices where id = :deviceId");
    qry.bindValue(":deviceId",eventId);
    qry.exec();
    if(qry.next())
    {
        unknownDevice = false;
    }

    if (unknownDevice == false)
    {
        qry.prepare("UPDATE commandData SET lastCommand = :eventCommand, lastValue = :eventValue,"
                    "lastUpdate = :lastUpdate WHERE dataId = :eventId");
        qry.bindValue(":eventCommand",eventCommand);
        qry.bindValue(":eventValue",eventValue);
        qry.bindValue(":lastUpdate",QDateTime::currentDateTime());
        qry.bindValue(":eventId",eventId);
        if(!qry.exec())
        {
            emit logEvent(tr("Failed to update event data from id %1, sql error %2")
                          .arg(eventId).arg(automationDatabase.lastError().text()));
        }
        if(qry.exec("SELECT * from deviceAction"))
        {
            while(qry.next())
            {
                int actionId = qry.value("actionId").toInt();
                int triggerValue = qry.value("triggerValue").toInt();
                int triggerCmd = qry.value("triggerCmd").toInt();
                int deviceId = qry.value("deviceId").toInt();
                if(eventId == deviceId && eventValue == triggerValue && eventCommand == triggerCmd)
                {
                    QSqlQuery query(QSqlDatabase::database(automationDatabase.connectionName()));
                    query.prepare("SELECT * FROM slaveAction WHERE actionId = :oldId");
                    query.bindValue(":oldId",actionId);
                    if(query.exec())
                    {
                        while(query.next())
                        {
                            int slaveId = query.value("deviceId").toInt();
                            int slaveCmd = query.value("cmd").toInt();
                            int slaveValue = query.value("value").toInt();
                            telldusApi->deviceCommand(slaveId,slaveCmd,slaveValue);
                        }
                    }
                    else
                    {
                        emit logEvent(tr("Failed to query database, sql error %1").arg(automationDatabase.lastError().text()));
                    }

                }
            }
        }
        else
        {
            emit logEvent(tr("Failed to query database, sql error %1").arg(automationDatabase.lastError().text()));
        }
        if (streamingEvents == true)
        {
            QByteArray block;
            QDataStream telegram(&block,QIODevice::WriteOnly);
            telegram.setVersion(QDataStream::Qt_5_3);
            telegram << eventId;
            telegram << eventCommand;
            telegram << eventValue;
            telegram << deviceType;
            telegram << QDateTime::currentDateTime();
            quint8 command = 'D';
            tcpServer->sendTelegram(command,telegram);
        }
    }
    else if (unknownDevice == true && lookingForDevices == true)
    {
        QByteArray block;
        QDataStream telegram(&block,QIODevice::WriteOnly);
        telegram.setVersion(QDataStream::Qt_5_3);
        telegram << eventId;
        telegram << eventCommand;
        telegram << eventValue;
        telegram << deviceType;
        telegram << QDateTime::currentDateTime();
        quint8 command = 'C';
        tcpServer->sendTelegram(command,telegram);
    }

}

void Manager::eventLogger(QString event)
{
    qDebug() << event;
}

void Manager::telldusInit()
{
    telldusApi = new TelldusApi(this);
    connect(telldusApi,SIGNAL(logEvent(QString)),this,SLOT(eventLogger(QString)));

    telldusApi->init();

    syncCoreWithDatabase();

}

void Manager::syncCoreWithDatabase()
{
    // Singleton instance of TelldusCoreAPI to forward events from the static callback functions
    disconnect(TelldusApiSingleton::Instance(),SIGNAL(deviceEvent(int,int,int,QString)),
               this,SLOT(deviceEvent(int,int,int,QString)));
    QSqlQuery qry(QSqlDatabase::database(automationDatabase.connectionName()));
    if(qry.exec("SELECT * FROM commandDevices ORDER BY id ASC"))
    {
        while(qry.next())
        {
            int id = qry.value("id").toInt();
            QString name = qry.value("name").toString();
            QString protocol = qry.value("protocol").toString();
            QString model = qry.value("model").toString();
            QString house = qry.value("house").toString();
            QString unit = qry.value("unit").toString();
            int idAssignedByCore = telldusApi->registerNewDevice(name,protocol,model,house,unit);
            if(idAssignedByCore <0)
            {
                break;
            }

            if(id != idAssignedByCore)
            {
                QSqlQuery query(QSqlDatabase::database(automationDatabase.connectionName()));
                query.prepare("UPDATE commandDevices SET id = :newId WHERE id = :oldId");
                query.bindValue(":newId",idAssignedByCore);
                query.bindValue(":oldId",id);
                if(!query.exec())
                {
                    emit logEvent(tr("Failed to change id from %1 to %2, for %3, sql error %4")
                                  .arg(id).arg(idAssignedByCore).arg(name)
                                  .arg(automationDatabase.lastError().text()));
                }
            }
        }
    }
    else
    {
        emit logEvent(tr("Failed to query database, sql error %1").arg(automationDatabase.lastError().text()));
    }
    if(!qry.exec("DELETE FROM commandData"))
    {
        emit logEvent(tr("Failed to query database, sql error %1").arg(automationDatabase.lastError().text()));
    }
    QList<int> idList;
    if(qry.exec("SELECT id FROM commandDevices"))
    {
        while(qry.next())
        {
            idList << qry.value("id").toInt();
        }
    }
    else
    {
        emit logEvent(tr("Failed to query database, sql error %1").arg(automationDatabase.lastError().text()));
    }

    qry.prepare("INSERT INTO commandData (dataId, supportedCommands, lastCommand, lastValue, lastUpdate)"
                "VALUES (:dataId, :supportedCommands, :lastCommand, :lastValue, :lastUpdate)");
    for(int i = 0; i<idList.size(); i++)
    {
        int supportedCommands, lastCommand, lastValue;
        telldusApi->getDeviceData(idList.at(i),supportedCommands,lastCommand,lastValue);
        qry.bindValue(":dataId",idList.at(i));
        qry.bindValue(":supportedCommands", supportedCommands);
        qry.bindValue(":lastCommand", lastCommand);
        qry.bindValue(":lastValue", lastValue);
        qry.bindValue(":lastUpdate",QDateTime::currentDateTime());
        if(!qry.exec())
        {
            emit logEvent(tr("Failed to query database, sql error %1").arg(automationDatabase.lastError().text()));
        }
    }

    // Singleton instance of TelldusCoreAPI to forward events from the static callback functions
    connect(TelldusApiSingleton::Instance(),SIGNAL(deviceEvent(int,int,int,QString)),
            this,SLOT(deviceEvent(int,int,int,QString)));
}

void Manager::processTelegram(quint8 command, QDataStream &in)
{
    switch (command) {
    case 'A': //Reload device database
        syncCoreWithDatabase();
        break;

    case 'B': // Device command from client
        int deviceId; int deviceCmd; int deviceValue;
        in >> deviceId;
        in >> deviceCmd;
        in >> deviceValue;
        telldusApi->deviceCommand(deviceId,deviceCmd,deviceValue);
        break;

    case 'C': // Stream all unkown device events to client
        int lookStatus;
        in >> lookStatus;
        lookingForDevices = lookStatus;
        break;

    case 'D': // Stream all known device events to client
        int streamStatus;
        in >> streamStatus;
        streamingEvents = streamStatus;

    default:
        emit logEvent(tr("TCP Received command of unknown type %1").arg(command));
        break;
    }
}
