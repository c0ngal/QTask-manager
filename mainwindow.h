#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "task.h"

#include <QVector>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QLabel>


QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addTask();
    void markTaskDone(QWidget* taskWidget);

private:
    Ui::MainWindow *ui;
    QVBoxLayout *taskLayout;
    QWidget *taskContainer;

    QVector<Task> tasks;
    void saveTasks();
    void loadTasks();
    void renderTask(const Task& task);

    QTimer *checkTimer;
    QSystemTrayIcon *trayIcon;
    void checkDueTasks();

    QMap<QLabel*, QWidget*> labelToTaskMap;

    void renderAllTasks();
    void sortTasksByTime();
    QPushButton *sortButton;

    void updateTaskList();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

};

#endif // MAINWINDOW_H
