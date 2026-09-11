#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 连接数据库
    db = QSqlDatabase::addDatabase("QMYSQL"); // 加载 MySQL 数据库驱动

    db.setHostName("127.0.0.1");
    db.setUserName("root");
    db.setPassword("123456");
    db.setDatabaseName("mydatabase");

    if(db.open())
    {
        QMessageBox::information(this, "连接提示", "连接成功");

        // 初始化SQL表格模型
        m = new QSqlTableModel;
        m->setTable("student");

        ui->tableView->setModel(m);

    }
    else
    {
        QMessageBox::warning(this, "连接提示", "连接失败");
    }


}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pushButton_insert_clicked()
{
    // 获取行编辑器内容
    QString id = ui->lineEdit_id->text();
    QString name = ui->lineEdit_name->text();
    QString birth = ui->lineEdit_birth->text();

    QString sql = QString("insert into student values (%1, '%2', '%3');").arg(id).arg(name).arg(birth);

    QSqlQuery query;
    if(query.exec(sql))
    {
        QMessageBox::information(this, "插入提示", "插入成功");
    }
    else
    {
        QMessageBox::warning(this, "插入提示", "插入失败");
    }

}

void Widget::on_pushButton_find_clicked()
{
    QSqlQuery query;
    query.exec("select * from student");

    while(query.next())
    {
        qDebug() << query.value(0);
        qDebug() << query.value(1);
        qDebug() << query.value(2);
    }

    m->select();
}

