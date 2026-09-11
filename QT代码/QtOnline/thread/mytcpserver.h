#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpServer>

class MyTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit MyTcpServer(QObject *parent = 0);

protected:
    void incomingConnection(qintptr socketDescriptor);

signals:
    void signal_to_thread();

public slots:
};

#endif // MYTCPSERVER_H
