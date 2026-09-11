#include "clienthandler.h"

ClientHandler::ClientHandler(int sockfd, QObject *parent) : QObject(parent)
{
    this->socketDescriptor = sockfd;
    //socket = new QTcpSocket;
}

void ClientHandler::working()
{
    socket = new QTcpSocket;
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QTcpSocket::readyRead, this, &ClientHandler::recvInfo);
}

void ClientHandler::recvInfo()
{
    qDebug() << socket->readAll();

    while (1);
}
