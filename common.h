#ifndef COMMON_H
#define COMMON_H

enum SwitchCommandTypes
{
    TurnOn = 1,
    TurnOff = 2,
    Bell = 4,
    Toggle = 8,
    Dim = 16,
    Learn = 32,
    Execute = 64,
    Up = 128,
    Down = 256,
    Stop = 512,
    AllSwitchTypes = 1023
};

enum SensorCommandTypes
{
    Temperature = 1,
    Humidity = 2,
    RainRate = 4,
    RainTotal = 8,
    WindDirection = 16,
    WindAverage = 32,
    WindGust = 64,
    AllSensorTypes = 127
};

enum ErrorCodes
{
    Success = 0,
    NotFound = -1,
    PermissionDenied = -2,
    DeviceNotFound = -3,
    MethodNotSupported = -4,
    Communication = -5,
    ConnectingService = -6,
    UnknownResponse = -7,
    Syntax = -8,
    BrokenPipe = -9,
    CommunicatingService = -10,
    UnknownError = -99
};

#endif // COMMON_H
