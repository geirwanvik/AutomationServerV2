#include <QCoreApplication>
#include "manager.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Manager automationManager;

    return a.exec();
}
