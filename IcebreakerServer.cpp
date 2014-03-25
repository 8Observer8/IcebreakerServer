#include "IcebreakerServer.h"

#include <QtNetwork>
#include <iostream>

IcebreakerServer::IcebreakerServer() :
    tcpServer(0),
    networkSession(0),
    m_nextBlockSize(0)
{
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

    openDatabase();
    configurateNetwork();

    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
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

        QString inputData;
        in >> inputData;
        QStringList list = inputData.split(',');

        if (inputData == QString("currentValues")) {
            // Pass current values to Viewer
            sendToViewer(clientSocket, mCurrentValues);
        } else if ((list.size() == 3) && (list[0] == QString("getRequiredRange"))) { // From Range
            sendToViewerDataFromRange(clientSocket, inputData);
        } else if (inputData == QString("getAvailableRange")) {
            sendToViewerAvailableRange(clientSocket);
        } else {
            // Save current values of sensors
            mCurrentValues = inputData;

            // Pass current values to Viewer
            sendToViewer(clientSocket, mCurrentValues);

            // Save values to database
            saveValuesToDatabase(inputData);
        }

        //        if (str == QString("currentValues")) {
        //            QString answer;
        //            int value01 = sensor_01();
        //            int value02 = sensor_02();
        //            int value03 = sensor_03();
        //            int value04 = sensor_04();
        //            int value05 = sensor_05();

        //            // Send answer to Viewer
        //            answer = QString("%1,%2,%3,%4,%5").arg(value01).arg(value02).arg(value03).arg(value04).arg(value05);
        //            sendToViewer(clientSocket, answer);
        //        } else {
        //            std::cout << "Unknown command: " << str.toStdString() << std::endl;
        //        }

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

void IcebreakerServer::sendToViewerDataFromRange(QTcpSocket *clientSocket, const QString &inputData)
{
    QStringList list = inputData.split(QChar(','));

    QString beginRequired = list[1];
    QString endRequired = list[2];

    if (beginRequired.isEmpty() || endRequired.isEmpty()) {
        std::cerr << "Error in sendToViewerDataFromRange(): beginRequired or endRequired is empty" << std::endl;
        return;
    }

    QDateTime beginDateTime;
    beginDateTime = QDateTime::fromString(beginRequired);

    QDateTime endDateTime;
    endDateTime = QDateTime::fromString(endRequired);

    // Check dateTime
    if (!beginDateTime.isValid()) {
        std::cerr << "Error: beginDateTime is not valid." << std::endl;
        return;
    }

    if (!endDateTime.isValid()) {
        std::cerr << "Error: endDateTime is not valid." << std::endl;
        return;
    }

    qint64 begin = beginDateTime.toMSecsSinceEpoch();
    qint64 end = endDateTime.toMSecsSinceEpoch();

    QSqlQuery query;
    QString answer;
    QString strQuery = QString("SELECT * FROM valuesOfSensors WHERE data_time>='%1' AND data_time<='%2';").arg(begin).arg(end);
    if (mDatabase.isOpen()) {
        if (!query.exec(strQuery)) {
            std::cerr << "Wrong query" << std::endl;
            return;
        } else {
            while (query.next()) {
                QString name = QString(query.value(3).toString());
                QString value = QString(query.value(1).toString());
                answer += QString("%1,%2;").arg(name).arg(value);
            }
        }
    } else {
        std::cerr << "No connection to Database." << std::endl;
        return;
    }

    sendToViewer(clientSocket, answer);
}

void IcebreakerServer::sendToViewerAvailableRange(QTcpSocket *clientSocket)
{
    QString answer = "alailableRange;";
    QSqlQuery query;
    QString strQuery;
    if (mDatabase.isOpen()) {
        // Time of first data
        strQuery = QString("SELECT data_time FROM valuesOfSensors LIMIT 1");
        if (!query.exec(strQuery)) {
            std::cerr << "Wrong query." << std::endl;
            return;
        } else {
            if (query.next()) {
                QDateTime time;
                time.setMSecsSinceEpoch(query.value(0).toString().toULongLong());
                answer += time.toString();
                answer += ";";
            }
        }

        // Time of last data
        strQuery = QString("SELECT data_time FROM valuesOfSensors WHERE ID = (SELECT MAX(ID) FROM valuesOfSensors);");
        if (!query.exec(strQuery)) {
            std::cerr << "Wrong query." << std::endl;
            return;
        } else {
            if (query.next()) {
                QDateTime time;
                time.setMSecsSinceEpoch(query.value(0).toULongLong());
                answer += time.toString();
            }
        }
    } else {
        std::cerr << "No connection to Database." << std::endl;
        return;
    }

    sendToViewer(clientSocket, answer);
}

void IcebreakerServer::openDatabase()
{
    mDatabase = QSqlDatabase::addDatabase("QSQLITE");
    QString path = QDir::currentPath()+QString("/database.sqlite");
    mDatabase.setDatabaseName(path);

    QFileInfo file(path);

    if (file.isFile()) {
        if (!mDatabase.open()) {
            std::cerr << "Database File was not opened." << std::endl;
        }
    } else {
        std::cerr << "Database File does not exist." << std::endl;
    }
}

void IcebreakerServer::configurateNetwork()
{
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
}

void IcebreakerServer::saveValuesToDatabase(const QString &inputData)
{
    QStringList sensorsList = inputData.split(';');
    for (int i = 0; i < sensorsList.size(); ++i) {
        QStringList list = sensorsList[i].split(',');
        if (list.size() == 2) {
            // Save to database
            if (mDatabase.isOpen()) {
                QSqlQuery query;
                for (int j = 0; j < list.size(); ++j) {
                    QString strQuery(QString("INSERT INTO valuesOfSensors(value, data_time, sensor_name) VALUES ('%1','%2', '%3')").arg(list[1]).arg(QDateTime::currentDateTime().toMSecsSinceEpoch()).arg(list[0]));
                    if (!query.exec(strQuery)) {
                        std::cerr << "Wrong query" << std::endl;
                        return;
                    }
                }
            } else {
                std::cerr << "No connection to Database." << std::endl;
                return;
            }
        }
    }
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
    std::cout << "Port Number: " << tcpServer->serverPort() << std::endl;
}
