#ifndef TELLDUSAPI_H
#define TELLDUSAPI_H

#include <QObject>
#include <QtSql>
#include "telldus-core.h"
#include "singleton.h"
#include "common.h"

class TelldusApi : public QObject
{
    Q_OBJECT
public:
    explicit TelldusApi(QObject *parent = 0);
    ~TelldusApi();

    void init();

    int registerNewDevice(QString name, QString protocol, QString model, QString house, QString unit);
    void getDeviceData(const int id, int &supportedCommands, int &lastCommand, int &lastValue);


signals:
    void deviceEvent(int eventId, int eventCommand, int eventValue, QString deviceType);
    void rawEvent(QString msg);
    void logEvent(QString msg);


private slots:

public slots:
    void deviceCommand(int id, int command, int value);

private:
    void errorCode(int id, int code);
    void removePreviousConfig();
    void removeDevice(int id);

    static void WINAPI rawDataEventCallback(const char *data, int controllerId, int, void *);
    static void WINAPI controllerEventCallback(int controllerId, int changeEvent, int changeType, const char *newValue, int callbackId, void *);
    static void WINAPI sensorEventCallback(const char *protocol, const char *model, int sensorId, int dataType, const char *value, int, int, void *);
    static void WINAPI deviceEventCallback(int deviceId, int method, const char *data, int, void *);
    static void WINAPI deviceChangeEventCallback(int deviceId, int changeEvent, int changeType, int, void *);

    int rawDataEventId, sensorEventId, deviceEventId, deviceChangedEventId, controllerEventId;
};

//Global variable
typedef Singleton<TelldusApi> TelldusApiSingleton;

#endif // TELLDUSAPI_H
