#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 使用 QSS 设置背景，只作用于当前窗口，不影响子控件
    setAttribute(Qt::WA_StyledBackground, true);
    this->setStyleSheet(R"(
        QWidget#Widget {
            border-image: url(:/images/client-background.png) 0 0 0 0 stretch stretch;
        }

        QLabel {
            color: white;
            background: transparent;
        }

        QLineEdit {
            color: #101543;
            background-color: rgba(255, 255, 255, 220);
            border: 1px solid rgba(100, 200, 255, 190);
            border-radius: 10px;
            padding: 0 10px;
        }

        QPushButton {
            color: white;
            background-color: #11cc39;
            border: 1px solid rgba(100, 200, 255, 210);
            border-radius: 10px;
            padding: 5px 20px;
        }

        QPushButton:hover {
            background-color: rgba(0, 180, 230, 220);
        }

        QPushButton:pressed {
            background-color: rgba(0, 100, 150, 230);
        }
    )");

    // 初始化 QTcpSocket
    tcpSocket = new QTcpSocket;
}

Widget::~Widget()
{
    delete ui;
    delete tcpSocket;
}

void Widget::on_pushButton_connect_clicked()
{
    // 获取用户输入的IP地址和端口号
    QString ipAddress = ui->lineEdit_ip->text();
    quint16 port = ui->lineEdit_port->text().toUShort();

    // 尝试连接到服务器
    tcpSocket->connectToHost(QHostAddress(ipAddress), port);

    // Qt 5.12 使用 error 信号；QOverload 用于选择带 SocketError 参数的重载。
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this,
            [this](QAbstractSocket::SocketError socketError) {
                Q_UNUSED(socketError);
                QMessageBox::critical(
                    this, "连接失败",
                    QString("无法连接到服务器：%1").arg(tcpSocket->errorString()));
    });

    // 连接成功后，您可以在这里处理连接成功的逻辑
    connect(tcpSocket, &QTcpSocket::connected, this, [this]() {
        QMessageBox::information(this, "连接成功", "已成功连接到服务器！");

        this->hide(); // 隐藏当前窗口

        // 打开聊天窗口
        Chat *chatWindow = new Chat(tcpSocket);

        chatWindow->show();
    });

    // 连接断开后，您可以在这里处理连接失败的逻辑
    connect(tcpSocket, &QTcpSocket::disconnected, this, [this]() {
        QMessageBox::warning(this, "连接断开", "与服务器的连接已断开！");
    });

}

void Widget::on_pushButton_cancel_clicked()
{
    this->close();
}

void Widget::on_pushButton_send_clicked()
{
    // 获取用户输入的消息内容
    QString message = ui->textEdit->toPlainText();

    qDebug() << "Sending message to server:" << message;

    // 将消息发送到服务器
    tcpSocket->write(message.toUtf8());
}
