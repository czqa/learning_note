#ifndef ClientHandler_H
#define ClientHandler_H

#include <QByteArray>
#include <QObject>
#include <QTcpSocket>

class ClientHandler : public QObject
{
    Q_OBJECT

public:
    explicit ClientHandler(QTcpSocket *socket, QObject *parent = nullptr);

signals:
    void dataReceived(const QByteArray &data);

public slots:
    void working();

private:
    QTcpSocket *m_socket;
};

#endif // ClientHandler_H
