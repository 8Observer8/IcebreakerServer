#ifndef ICEBREAKERSERVER_H
#define ICEBREAKERSERVER_H

#include <QObject>

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

    int sensor_01();
    int sensor_02();
    int sensor_03();
    int sensor_04();
    int sensor_05();
};

#endif // ICEBREAKERSERVER_H
