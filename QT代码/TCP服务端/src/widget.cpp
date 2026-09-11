#include "widget.h"
#include "ui_widget.h"

#include <QThread>
#include "ClientHandler.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    server = new QTcpServer(this);

    // 监听6666端口
    server->listen(QHostAddress::AnyIPv4, 6666);

    // 等待新的客户端连接
    connect(server, &QTcpServer::newConnection, this, [this](){

        // 建立TCP 连接
        QTcpSocket *clientSocket = server->nextPendingConnection();

        // 读取IP和端口号
        clientSocket->peerAddress();
        clientSocket->peerPort();

        ui->lineEdit_ip->setText(clientSocket->peerAddress().toString());
        ui->lineEdit_port->setText(QString::number(clientSocket->peerPort()));

        // 创建新线程
        QThread *thread = new QThread(this);
        ClientHandler *client = new ClientHandler(clientSocket);
        client->moveToThread(thread);

        // 等待数据接受完成
        connect(client, &ClientHandler::dataReceived,this, [this](const QByteArray &data) {
            ui->textEdit->append(QString(data));
        });

        thread->start();
         
    });
}

Widget::~Widget()
{
    server->close();

    delete ui;
}
