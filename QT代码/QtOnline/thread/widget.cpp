#include "widget.h"
#include "ui_widget.h"
#include <QThread>
#include "clienthandler.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    server = new MyTcpServer;

    server->listen(QHostAddress::AnyIPv4, PORT);
}

Widget::~Widget()
{
    delete ui;
}

//void Widget::newClientHandler()
//{
//    ui->ipLineEdit->setText(socket->peerAddress().toString());
//    ui->portLineEdit->setText(QString::number(socket->peerPort()));
//}

//void Widget::clientInfoSlot()
//{
//    //获取信号的发出者
//    QTcpSocket *s = (QTcpSocket *)sender();
//    ui->mainLineEdit->setText(QString(s->readAll()));
//}

void Widget::threadSlot(QByteArray b)
{
    ui->mainLineEdit->setText(QString(b));
}

