#include "IcebreakerServer.h"

#include <QtNetwork>
#include <iostream>

IcebreakerServer::IcebreakerServer() :
    tcpServer(0),
    networkSession(0),
    m_nextBlockSize(0)
{
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        // Get saved network configuration
        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) !=
                QNetworkConfiguration::Discovered) {
            config = manager.defaultConfiguration();
        }

        networkSession = new QNetworkSession(config, this);
        connect(networkSession, SIGNAL(opened()), this, SLOT(sessionOpened()));

        //statusLabel->setText(tr("Opening network session."));
        networkSession->open();
    } else {
        sessionOpened();
    }

    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

void IcebreakerServer::sessionOpened()
{
    // Save the used configuration
    if (networkSession) {
        QNetworkConfiguration config = networkSession->configuration();
        QString id;
        if (config.type() == QNetworkConfiguration::UserChoice)
            id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
        else
            id = config.identifier();

        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
        settings.endGroup();
    }

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen()) {
        //        QMessageBox::critical(this, tr("Fortune Server"),
        //                              tr("Unable to start the server: %1.")
        //                              .arg(tcpServer->errorString()));
        std::cerr << "Error: Unable to start the server " <<
                     tcpServer->errorString().toStdString() << std::endl;
        return;
    }

    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
                ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }

    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

    //    statusLabel->setText(tr("The server is running on\n\nIP: %1\nport: %2\n\n"
    //                            "Run the Fortune Client example now.")
    //                         .arg(ipAddress).arg(tcpServer->serverPort()));
    std::cout << "The server is running on" << std::endl;
    std::cout << "IP: " << ipAddress.toStdString() << std::endl;
    std::cout << "Number Of Port: " << tcpServer->serverPort() << std::endl;
}

void IcebreakerServer::newConnection()
{
    tcpSocket = tcpServer->nextPendingConnection();
    connect(tcpSocket, SIGNAL(disconnected()),
            tcpSocket, SLOT(deleteLater())
            );
    connect(tcpSocket, SIGNAL(readyRead()),
            this, SLOT(readyRead())
            );
}

void IcebreakerServer::readyRead()
{
    QTcpSocket* clientSocket = (QTcpSocket*)sender();
    QDataStream in(clientSocket);
    in.setVersion(QDataStream::Qt_4_7);
    for (;;) {
        if (!m_nextBlockSize) {
            if (clientSocket->bytesAvailable() < (int)sizeof(quint16)) {
                break;
            }
            in >> m_nextBlockSize;
        }

        if (clientSocket->bytesAvailable() < m_nextBlockSize) {
            break;
        }

        QString str;
        in >> str;

        if (str == QString("currentValues")) {
            QString answer;
            int value01 = sensor_01();
            int value02 = sensor_02();
            int value03 = sensor_03();
            int value04 = sensor_04();
            int value05 = sensor_05();
            answer = QString("%1,%2,%3,%4,%5").arg(value01).arg(value02).arg(value03).arg(value04).arg(value05);
            sendToViewer(clientSocket, answer);
        } else {
            std::cout << "Unknown command: " << str.toStdString() << std::endl;
        }

        m_nextBlockSize = 0;
    }
}

void IcebreakerServer::sendToViewer(QTcpSocket *socket, const QString &str)
{
    QByteArray  arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_7);

    out << quint16(0) << str;

    out.device()->seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));

    socket->write(arrBlock);
}

int IcebreakerServer::sensor_01()
{
    return qrand() % 10 + 10;
}

int IcebreakerServer::sensor_02()
{
    return qrand() % 10 + 20;
}

int IcebreakerServer::sensor_03()
{
    return qrand() % 10 + 30;
}

int IcebreakerServer::sensor_04()
{
    return qrand() % 10 + 40;
}

int IcebreakerServer::sensor_05()
{
    return qrand() % 10 + 50;
}
