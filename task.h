#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

struct Task {
    QString text;
    QString note;
    QDateTime dueTime;
    bool notified = false;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["text"] = text;
        obj["note"] = note;
        obj["dueTime"] = dueTime.toString(Qt::ISODate);
        obj["notified"] = notified;
        return obj;
    }

    static Task fromJson(const QJsonObject& obj) {
        Task task;
        task.text = obj["text"].toString();
        task.note = obj["note"].toString();
        task.dueTime = QDateTime::fromString(obj["dueTime"].toString(), Qt::ISODate);
        task.notified = obj["notified"].toBool();
        return task;
    }

};


#endif // TASK_H
