#ifndef ICEBREAKERSERVER_H
#define ICEBREAKERSERVER_H

#include <QObject>
#include <QtSql>

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
class QNetworkSession;
QT_END_NAMESPACE

class IcebreakerServer : public QObject
{
    Q_OBJECT
public:
    IcebreakerServer();

private slots:
    void sessionOpened();
    void newConnection();
    void readyRead();
    void sendToViewer(QTcpSocket *pSocket, const QString &str);

private:
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket;
    QNetworkSession *networkSession;
    quint16 m_nextBlockSize;
    QSqlDatabase database;

    int sensor_01();
    int sensor_02();
    int sensor_03();
    int sensor_04();
    int sensor_05();
};

#endif // ICEBREAKERSERVER_H
