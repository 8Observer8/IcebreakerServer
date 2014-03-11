#include <QCoreApplication>
#include "IcebreakerServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    IcebreakerServer server;

    return a.exec();
}
