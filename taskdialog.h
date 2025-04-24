#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QVBoxLayout>

class TaskDialog : public QDialog {
    Q_OBJECT

public:
    TaskDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Новое напоминание");
        QVBoxLayout *layout = new QVBoxLayout(this);

        textEdit = new QLineEdit(this);
        textEdit->setPlaceholderText("Текст задачи");
        layout->addWidget(textEdit);

        dateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
        dateTimeEdit->setCalendarPopup(true);
        layout->addWidget(dateTimeEdit);

        QPushButton *okButton = new QPushButton("Добавить", this);
        layout->addWidget(okButton);

        connect(okButton, &QPushButton::clicked, this, &TaskDialog::accept);
    }

    QString getText() const { return textEdit->text(); }
    QDateTime getDateTime() const { return dateTimeEdit->dateTime();

    }


private:
    QLineEdit *textEdit;
    QDateTimeEdit *dateTimeEdit;
};


#endif // TASKDIALOG_H
