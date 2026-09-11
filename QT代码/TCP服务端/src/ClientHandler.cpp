#include "ClientHandler.h"

#include <QTcpSocket>

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
{
    m_socket->setParent(this);

    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::working);
}

void ClientHandler::working()
{
    // 接收数据
    const QByteArray data = m_socket->readAll();
    
    if (!data.isEmpty())
        emit dataReceived(data);
}
