#ifndef CHAT_H
#define CHAT_H

#include <QWidget>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class Chat; }
QT_END_NAMESPACE

class Chat : public QWidget
{
    Q_OBJECT

public:
    explicit Chat(QTcpSocket *tcpSocket, QWidget *parent = nullptr);
    ~Chat() override;

private slots:
    void on_pushButton_S_clicked();
    void on_pushButton_C_clicked();

private:
    Ui::Chat *ui;

    QTcpSocket *tcpSocket;
};

#endif // CHAT_H
