#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "taskdialog.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QWidget* central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    //добавление задачи
    QPushButton *addButton = new QPushButton("+", this);
    addButton->setFixedSize(30, 30);
    mainLayout->addWidget(addButton, 0, Qt::AlignRight);

    //контейнер для задач
    taskContainer = new QWidget(this);
    taskLayout = new QVBoxLayout(taskContainer);
    taskLayout->setAlignment(Qt::AlignTop);
    taskContainer->setLayout(taskLayout);

    mainLayout->addWidget(taskContainer);
    central->setLayout(mainLayout);
    setCentralWidget(central);

    loadTasks();

    connect(addButton, &QPushButton::clicked, this, &MainWindow::addTask);

    //добавить какой-нибудь логотип
    trayIcon = new QSystemTrayIcon(this);
    //trayIcon->setIcon(QIcon(""));
    trayIcon->show();

    checkTimer = new QTimer(this);
    connect(checkTimer, &QTimer::timeout, this, &MainWindow::checkDueTasks);
    checkTimer->start(30000);

    sortButton = new QPushButton("Сортировать по времени", this);
    sortButton->setStyleSheet("color: white; background-color: #444; border: 1px solid gray;");
    taskLayout->addWidget(sortButton);



    connect(sortButton, &QPushButton::clicked, this, &MainWindow::sortTasksByTime);


}

void MainWindow::addTask() {
    bool ok;

    //запрос на ввод напоминания
    QString text = QInputDialog::getText(this, tr("Напоминание"), tr("Введите текст задачи:"), QLineEdit::Normal, "", &ok);
    if (!ok || text.isEmpty()) return;

    //запрос на ввод заметки
    QString note = QInputDialog::getText(this, tr("Заметка"), tr("Введите заметку (необязательно):"), QLineEdit::Normal, "", &ok);
    if (!ok) note = "";  // Если заметка не введена, оставляем пустую строку

    //запрос на ввод времени
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Выбор времени"), tr("Хотите выбрать время для задачи?"), QMessageBox::Yes | QMessageBox::No);
    QDateTime dueTime;
    if (reply == QMessageBox::Yes) {

        QDateTimeEdit* dateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
        dateTimeEdit->setCalendarPopup(true);  //всплывающее окно календаря
        dateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");

        QDialog* timeDialog = new QDialog(this);
        QVBoxLayout* layout = new QVBoxLayout(timeDialog);
        layout->addWidget(dateTimeEdit);

        QPushButton* okButton = new QPushButton(tr("Ок"), timeDialog);
        QPushButton* cancelButton = new QPushButton(tr("Отмена"), timeDialog);
        layout->addWidget(okButton);
        layout->addWidget(cancelButton);

        connect(okButton, &QPushButton::clicked, timeDialog, &QDialog::accept);
        connect(cancelButton, &QPushButton::clicked, timeDialog, &QDialog::reject);

        //ожидание, пока пользователь выберет время
        if (timeDialog->exec() == QDialog::Accepted) {
            dueTime = dateTimeEdit->dateTime();

            //проверка на корректность времени
            if (dueTime < QDateTime::currentDateTime()) {
                QMessageBox::warning(this, tr("Ошибка"), tr("Время задачи не может быть в прошлом."));
                delete timeDialog;
                return;
            }
        }

        delete timeDialog;
    } else {
        //если время не выбрано, ставим пустое время
        dueTime = QDateTime();
    }

    //создание задачи
    Task task;
    task.text = text;
    task.note = note;
    task.dueTime = dueTime;

    //добавление задачи в список и обновление интерфейса
    tasks.push_back(task);
    renderTask(task);
    saveTasks();
}

void MainWindow::markTaskDone(QWidget *taskWidget) {

    taskLayout->removeEventFilter(taskWidget);
    taskWidget->deleteLater();

}

void MainWindow::saveTasks() {
    QJsonArray taskArray;
    for (const Task& task : tasks) {
        taskArray.append(task.toJson());
    }

    QJsonDocument doc(taskArray);
    QFile file("tasks.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void MainWindow::loadTasks() {
    QFile file("tasks.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            QJsonArray array = doc.array();
            for (const QJsonValue& value : array) {
                Task task = Task::fromJson(value.toObject());
                tasks.push_back(task);
                renderTask(task);
            }
        }
    }
}

void MainWindow::renderAllTasks() {
    QLayoutItem* item;
    while ((item = taskLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
        if (!a.dueTime.isValid() && b.dueTime.isValid()) return false;
        if (a.dueTime.isValid() && !b.dueTime.isValid()) return true;
        return a.dueTime < b.dueTime;
    });

    for (const Task& task : tasks) {
        renderTask(task);
    }
}

void MainWindow::sortTasksByTime() {
    //сортировка задач по dueTime
    std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
        //если оба dueTime заданы, сравниваем напрямую
        if (a.dueTime.isValid() && b.dueTime.isValid())
            return a.dueTime < b.dueTime;

        //задачи без dueTime идут в конец
        if (!a.dueTime.isValid()) return false;
        if (!b.dueTime.isValid()) return true;

        return false;
    });

    //удаление старых виджетов из layout
    QLayoutItem *item;
    while ((item = taskLayout->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    //отображение задач в отсортированном порядке
    for (const Task &task : tasks) {
        renderTask(task);
    }
}


void MainWindow::renderTask(const Task& task) {
    QWidget *taskWidget = new QWidget(this);
    QVBoxLayout *taskVBox = new QVBoxLayout(taskWidget);

    QWidget *topRow = new QWidget(this);
    QHBoxLayout *topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);


    QPushButton *circleButton = new QPushButton(this);
    circleButton->setFixedSize(15, 15);
    circleButton->setStyleSheet("border-radius: 10px; border: 2px solid gray; background-color: white;");
    topLayout->addWidget(circleButton);

    QLabel *labelText = new QLabel(task.text, this);
    labelText->setStyleSheet("color: white;");
    topLayout->addWidget(labelText);
    topRow->setLayout(topLayout);

    //двойной клик для редактирования напоминания
    labelText->setTextInteractionFlags(Qt::TextEditorInteraction);
    labelText->installEventFilter(this);

    //заметка
    QLabel *noteLabel = new QLabel(task.note.isEmpty() ? "Нет заметки" : task.note, this);
    noteLabel->setStyleSheet("color: gray; font-style: italic;");
    noteLabel->setTextInteractionFlags(Qt::TextEditorInteraction); // Это позволит редактировать текст заметки
    noteLabel->installEventFilter(this);

    //время
    QLabel *timeLabel = new QLabel(task.dueTime.isValid() ? task.dueTime.toString("dd.MM.yyyy hh:mm") : "Без времени", this);
    if (task.dueTime.isValid() && task.dueTime > QDateTime::currentDateTime()) {
        timeLabel->setStyleSheet("color: gray;");
    } else {
        timeLabel->setStyleSheet("color: red;");
    }



    //двойной клик для редактирования времени
    timeLabel->setTextInteractionFlags(Qt::TextEditorInteraction);
    timeLabel->installEventFilter(this);

    connect(timeLabel, &QLabel::linkActivated, this, [=]() {
        QDateTimeEdit *dateTimeEdit = new QDateTimeEdit(task.dueTime, this);
        dateTimeEdit->setDisplayFormat("dd.MM.yyyy hh:mm");
        dateTimeEdit->setCalendarPopup(true);

        // Когда пользователь выбирает новое время
        connect(dateTimeEdit, &QDateTimeEdit::editingFinished, [=]() {
            QDateTime newDateTime = dateTimeEdit->dateTime();
            if (newDateTime != task.dueTime) {
                Task updatedTask = task;
                updatedTask.dueTime = newDateTime;

                // Обновляем метку времени
                timeLabel->setText(newDateTime.toString("dd.MM.yyyy hh:mm"));

                // Обновляем стиль метки времени в зависимости от нового времени
                if (newDateTime > QDateTime::currentDateTime()) {
                    timeLabel->setStyleSheet("color: gray;");
                } else {
                    timeLabel->setStyleSheet("color: red;");
                }

                // Обновляем задачу в списке и сохраняем
                for (Task &t : tasks) {
                    if (t.text == task.text) {
                        t.dueTime = newDateTime;
                        break;
                    }
                }

                saveTasks();  // Сохранение изменений в задаче
            }
            dateTimeEdit->deleteLater();  // Удаление временного виджета
        });

        dateTimeEdit->move(timeLabel->mapToGlobal(QPoint(0, 0)));
        dateTimeEdit->show();
    });

    //добавление виджетов в layout
    taskVBox->addWidget(topRow);  //сначала напоминание
    taskVBox->addWidget(noteLabel); //затем заметка
    taskVBox->addWidget(timeLabel); //потом время

    taskWidget->setLayout(taskVBox);
    taskLayout->addWidget(taskWidget);

    // Обработчик для кнопки удаления задачи
    connect(circleButton, &QPushButton::clicked, this, [=]() {
        taskLayout->removeWidget(taskWidget);
        taskWidget->deleteLater();
        tasks.erase(std::remove_if(tasks.begin(), tasks.end(), [&](const Task &t) {
                        return t.text == labelText->text() && t.dueTime == task.dueTime;
                    }), tasks.end());
        saveTasks();
    });

    labelToTaskMap[labelText] = taskWidget;
    labelToTaskMap[noteLabel] = taskWidget;
}

void MainWindow::checkDueTasks() {
    QDateTime now = QDateTime::currentDateTime();

    for (const Task& task : tasks) {
        if (!task.notified && task.dueTime <= now) {
            trayIcon->showMessage("Напоминание", task.text, QSystemTrayIcon::Information, 10000);

            const_cast<Task&>(task).notified = true;
            //renderTask(task);
            updateTaskList();
        }
    }
}

void MainWindow::updateTaskList() {
    saveTasks();
    renderAllTasks();
}


bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonDblClick) {
        QLabel *label = qobject_cast<QLabel*>(obj);
        if (label) {
            bool ok;
            QString newText = QInputDialog::getText(this, "Редактировать напоминание", "Измнить текст:", QLineEdit::Normal,
                                                    label->text(), &ok);
            if (ok && !newText.isEmpty()) {
                label->setText(newText);

                for (Task& t : tasks) {
                    if (t.text == label->text()) {
                        t.text = newText;
                        break;
                    }
                }
                saveTasks();
            }
        }
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}





MainWindow::~MainWindow()
{
    delete ui;
}
