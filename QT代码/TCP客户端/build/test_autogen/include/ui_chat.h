/********************************************************************************
** Form generated from reading UI file 'chat.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHAT_H
#define UI_CHAT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Chat
{
public:
    QTextEdit *textEdit;
    QLabel *label;
    QPushButton *pushButton_S;
    QPushButton *pushButton_C;

    void setupUi(QWidget *Chat)
    {
        if (Chat->objectName().isEmpty())
            Chat->setObjectName(QString::fromUtf8("Chat"));
        Chat->resize(880, 554);
        textEdit = new QTextEdit(Chat);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));
        textEdit->setGeometry(QRect(190, 70, 461, 131));
        label = new QLabel(Chat);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(50, 90, 131, 101));
        pushButton_S = new QPushButton(Chat);
        pushButton_S->setObjectName(QString::fromUtf8("pushButton_S"));
        pushButton_S->setGeometry(QRect(270, 270, 89, 25));
        pushButton_C = new QPushButton(Chat);
        pushButton_C->setObjectName(QString::fromUtf8("pushButton_C"));
        pushButton_C->setGeometry(QRect(500, 270, 89, 25));

        retranslateUi(Chat);

        QMetaObject::connectSlotsByName(Chat);
    } // setupUi

    void retranslateUi(QWidget *Chat)
    {
        Chat->setWindowTitle(QApplication::translate("Chat", "\350\201\212\345\244\251\347\252\227\345\217\243", nullptr));
        label->setText(QApplication::translate("Chat", "<html><head/><body><p align=\"center\"><span style=\" font-size:16pt;\">\350\257\267\350\276\223\345\205\245\346\225\260\346\215\256</span></p></body></html>", nullptr));
        pushButton_S->setText(QApplication::translate("Chat", "\345\217\221\351\200\201", nullptr));
        pushButton_C->setText(QApplication::translate("Chat", "\346\270\205\347\251\272", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Chat: public Ui_Chat {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHAT_H
