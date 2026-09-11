#include "mytcpserver.h"
#include "clienthandler.h"
#include <QThread>

MyTcpServer::MyTcpServer(QObject *parent) : QTcpServer(parent)
{

}

//有客户端连接，会触发该函数
void MyTcpServer::incomingConnection(qintptr socketDescriptor)
{
    //启动线程
    QThread *subThread = new QThread;             //线程对象
    /*socket不能跨线程使用*/
    ClientHandler *myClient = new ClientHandler(socketDescriptor);  //事件对象

    myClient->moveToThread(subThread);

    subThread->start();                           //启动线程

    connect(this, &MyTcpServer::signal_to_thread, myClient, &ClientHandler::working);
    emit signal_to_thread();
}
