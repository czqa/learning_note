#include "mythread.h"

myThread::myThread(QTcpSocket *s)
{
    socket = s;
}

void myThread::run()
{
    /*子线程任务代码必须放在run函数里面*/
    connect(socket, &QTcpSocket::readyRead, this, &myThread::clientInfoSlot);
}

//不能在其他类里面操作界面
void myThread::clientInfoSlot()
{
    //qDebug() << socket->readAll();
    QByteArray ba = socket->readAll();
    emit sendToWidget(ba);      //发送信号

    //while (1);
}
