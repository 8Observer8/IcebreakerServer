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
    void sendToViewerDataFromRange(QTcpSocket *clientSocket, const QString &list);
    void sendToViewerAvailableRange(QTcpSocket *clientSocket);

private:
    void openDatabase();
    void configurateNetwork();
    void saveValuesToDatabase(const QString &inputData);
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket;
    QNetworkSession *networkSession;
    quint16 m_nextBlockSize;
    QString mCurrentValues;
    QSqlDatabase mDatabase;
};

#endif // ICEBREAKERSERVER_H
