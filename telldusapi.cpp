#include "telldusapi.h"

const int DATA_LENGTH = 20;
int callbackRefs[4];

TelldusApi::TelldusApi(QObject *parent) : QObject(parent)
{

}

TelldusApi::~TelldusApi()
{
    tdUnregisterCallback(callbackRefs[0]);
    tdUnregisterCallback(callbackRefs[1]);
    tdUnregisterCallback(callbackRefs[2]);
    tdUnregisterCallback(callbackRefs[3]);

    tdClose();
}

void TelldusApi::init()
{
    tdInit();

    removePreviousConfig();

    callbackRefs[0] = tdRegisterDeviceEvent( (TDDeviceEvent) &deviceEventCallback, 0);
    callbackRefs[1] =tdRegisterSensorEvent( (TDSensorEvent) &sensorEventCallback, 0);
    callbackRefs[2] =tdRegisterRawDeviceEvent( (TDRawDeviceEvent) &rawDataEventCallback, 0);
    callbackRefs[3] =tdRegisterDeviceChangeEvent( (TDDeviceChangeEvent) &deviceChangeEventCallback, 0);
}

void TelldusApi::removePreviousConfig()
{
    QByteArray devList;

    int numberOfDevices = tdGetNumberOfDevices();

    for (int i = 0; i < numberOfDevices; i++)
    {
      int id = tdGetDeviceId(i);
      devList.append(id);
    }

    for(int i = 0; i<devList.size(); i++)
    {
      tdRemoveDevice(devList.at(i));
    }

    QTime myClock;
    myClock.start();

    while(tdGetNumberOfDevices() != 0 && myClock.elapsed() < 5000)
    {
        // Wait for telldusd to update or 5 sec
    }
    if(tdGetNumberOfDevices() == 0)
    {
        emit logEvent(tr("Previous config successfully removed"));
        return;
    }
    emit logEvent(tr("Failed to remove all devices in config, %1 devices remaining").arg(tdGetNumberOfDevices()));
}


int TelldusApi::registerNewDevice(QString name, QString protocol, QString model, QString house, QString unit)
{
    // Id set by core
    int id = tdAddDevice();
    if(id < 0) // If error, the error code is returned as the device id, negative number
    {
        errorCode(-1,id);
        return -1;
    }

    bool returnValue;

    // Set everything else
    returnValue = tdSetName(id,name.toUtf8().constData());

    if(!returnValue)
    {
        emit logEvent(tr("Failed to set name for device %1").arg(id));
        return -1;
    }

    returnValue = tdSetProtocol(id,protocol.toUtf8().constData());
    if(!returnValue)
    {
        emit logEvent(tr("Failed to set protocol for device %1").arg(id));
        return -1;
    }

    returnValue = tdSetModel(id,model.toUtf8().constData());
    if(!returnValue)
    {
        emit logEvent(tr("Failed to set model for device %1").arg(id));
        return -1;
    }

    returnValue = tdSetDeviceParameter(id,"house",house.toUtf8().constData());
    if(!returnValue)
    {
        emit logEvent(tr("Failed to set house for device %1").arg(id));
        return -1;
    }

    returnValue = tdSetDeviceParameter(id,"unit",unit.toUtf8().constData());
    if(!returnValue)
    {
        emit logEvent(tr("Failed to set unit for device %1").arg(id));
        return -1;
    }

    return id;
}

void TelldusApi::getDeviceData(const int id, int &supportedCommands, int &lastCommand, int &lastValue)
{
    supportedCommands = tdMethods(id, AllSwitchTypes);
    lastCommand = tdLastSentCommand(id,supportedCommands);
    lastValue = atoi(tdLastSentValue(id));
}

void TelldusApi::removeDevice(int id)
{
    bool success;
    success = tdRemoveDevice(id);
    QString result;
    if(success)
    {
        result = tr("Device with id %1 removed from Telldus Core").arg(id);
    }
    else
    {
        result = tr("Could not remove device with id %1 from Telldus Core").arg(id);
    }
    emit logEvent(result);
}

void TelldusApi::deviceCommand(int id, int command, int value)
{
    char dimmerValue = value;
    int success = tdMethods(id,command);
    if(!(success & command))
    {
        QString msg = tr("Device with id %1 does not support command type %2").arg(id).arg(command);
        emit logEvent(msg);
        return;
    }
    switch(command)
    {
    case TurnOn:
        success = tdTurnOn(id);
        break;

    case TurnOff:
        success = tdTurnOff(id);
        break;

    case Bell:
        success = tdBell(id);
        break;

    case Toggle:
        //Not implemented by Telldus
        break;

    case Dim:
        success = tdDim(id,dimmerValue);
        break;

    case Learn:
        success = tdLearn(id);
        break;

    case Execute:
        success = tdExecute(id);
        break;

    case Up:
        success = tdUp(id);
        break;

    case Down:
        success = tdDown(id);
        break;

    case Stop:
        success = tdStop(id);
        break;
    }

    if(success != Success)
    {
        errorCode(id,success);
    }
}

void TelldusApi::errorCode(int id, int code)
{
    switch(code)
    {
    case Success:
        emit logEvent(tr("Error function called without error on id %1").arg(id));
        break;

    case NotFound:
        emit logEvent(tr("Error : Device id %1 TelldusApi not found").arg(id));
        break;

    case PermissionDenied:
        emit logEvent(tr("Error : Device id %1 TelldusApi permission denied").arg(id));
        break;

    case DeviceNotFound:
        emit logEvent(tr("Error : Device id %1 TelldusApi permission denied").arg(id));
        break;

    case MethodNotSupported:
        emit logEvent(tr("Error : Device id %1 The method is not supported by the device.").arg(id));
        break;

    case Communication:
        emit logEvent(tr("Error : Device id %1 Error when communicating with TelldusApi").arg(id));
        break;

    case ConnectingService:
        emit logEvent(tr("Error : Device id %1 The client library could not connect to the service").arg(id));
        break;

    case UnknownResponse:
        emit logEvent(tr("Error : Device id %1 The client library received a response from the service it did not understand").arg(id));
        break;

    case Syntax:
        emit logEvent(tr("Error : Device id %1 Input/command could not be parsed or didn't follow input rules").arg(id));
        break;

    case BrokenPipe:
        emit logEvent(tr("Error : Device id %1 Pipe broken during communication").arg(id));
        break;

    case CommunicatingService:
        emit logEvent(tr("Error : Device id %1 Timeout waiting for response from the Telldus Service").arg(id));
        break;

    case UnknownError:
        emit logEvent(tr("Error : Device id %1 An unknown error has occurred").arg(id));
        break;
    }
}

void WINAPI TelldusApi::deviceEventCallback(int deviceId, int method, const char *data, int, void*)
{
    static QString lastMsg;
    QString msg = tr("Device Event from id:%1; method:%2; data:%3;").arg(deviceId).arg(method).arg(data);

    static quint64 lastEvent = 0;
    quint64 thisEvent = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if(lastMsg == msg && lastEvent == thisEvent)
    {
        return;
    }
    lastMsg = msg;
    lastEvent = thisEvent;

    int eventValue = atoi(data);
    TelldusApiSingleton::Instance()->deviceEvent(deviceId, method, eventValue, "command");

    TelldusApiSingleton::Instance()->logEvent(msg);
}

void WINAPI TelldusApi::sensorEventCallback(const char *protocol, const char *model, int sensorId, int dataType, const char *value, int, int, void*)
{
    static QString lastMsg;
    QString msg = tr("Sensor Event from protocol:%1; model:%2; id:%3; datatype:%4; value:%5;").arg(protocol).arg(model).arg(sensorId).arg(dataType).arg(value);

    static quint64 lastEvent = 0;
    quint64 thisEvent = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if(lastMsg == msg && lastEvent == thisEvent)
    {
        return;
    }
    lastMsg = msg;
    lastEvent = thisEvent;

    int eventValue = atoi(value);
    TelldusApiSingleton::Instance()->deviceEvent(sensorId, dataType, eventValue, "sensor");

    TelldusApiSingleton::Instance()->logEvent(msg);
}

void WINAPI TelldusApi::rawDataEventCallback(const char *data, int controllerId, int, void*)
{
    static QString lastMsg;
    QString msg = tr("Raw Event from id:%1; data %2").arg(controllerId).arg(data);
    static quint64 lastEvent = 0;
    quint64 thisEvent = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if(lastMsg == msg && lastEvent == thisEvent)
    {
        return;
    }
    lastMsg = msg;
    lastEvent = thisEvent;
    TelldusApiSingleton::Instance()->rawEvent(tr("id:%1;%2").arg(controllerId).arg(data));

    TelldusApiSingleton::Instance()->logEvent(msg);
}

void WINAPI TelldusApi::deviceChangeEventCallback(int deviceId, int changeEvent, int changeType, int, void*)
{
    static QString lastMsg;
    QString msg = tr("Device Change Event from id:%1; change event:%2; change type:%3;").arg(deviceId).arg(changeEvent).arg(changeType);
    static quint64 lastEvent = 0;
    quint64 thisEvent = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if(lastMsg == msg && lastEvent == thisEvent)
    {
        return;
    }
    lastMsg = msg;
    lastEvent = thisEvent;

    TelldusApiSingleton::Instance()->logEvent(msg);
}

void WINAPI TelldusApi::controllerEventCallback(int controllerId, int changeEvent, int changeType, const char *newValue, int, void*)
{
    static QString lastMsg;
    QString msg = tr("Controller Event from controller id:%1; change event:%2; change type:%3; new value:%4;").arg(controllerId).arg(changeEvent).arg(changeType).arg(newValue);
    static quint64 lastEvent = 0;
    quint64 thisEvent = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if(lastMsg == msg && lastEvent == thisEvent)
    {
        return;
    }
    lastMsg = msg;
    lastEvent = thisEvent;

    TelldusApiSingleton::Instance()->logEvent(msg);
}
