#include "chat.h"
#include "ui_chat.h"

Chat::Chat(QTcpSocket *tcpSocket, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Chat)
    , tcpSocket(tcpSocket)
{
    ui->setupUi(this);
}

Chat::~Chat()
{
    delete ui;
}

void Chat::on_pushButton_S_clicked()
{
    // 获取用户输入的消息内容
    QString message = ui->textEdit->toPlainText();

    qDebug() << "Sending message to server:" << message;

    // 将消息发送到服务器
    tcpSocket->write(message.toUtf8());
}

void Chat::on_pushButton_C_clicked()
{
    ui->textEdit->clear();
}
