#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(int sockfd, QObject *parent = 0);

private slots:
    void recvInfo();

signals:

public slots:
    void working();

private:
    qintptr socketDescriptor;
    QTcpSocket *socket;
};

#endif // CLIENTHANDLER_H
