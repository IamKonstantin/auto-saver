#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <iostream>

#include <QFileDialog>
#include <QRegExp>
#include <QSettings>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QResizeEvent>

#include "applybutton.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settings(new QSettings("auto-saver.ini", QSettings::IniFormat, this))
    , last_modified(QDateTime::fromMSecsSinceEpoch(0))
    , changes_timer(new QTimer(this))
    , writing_timer(new QTimer(this))
{
    const QSize window_size = settings->value("window_size").toSize();
    if (window_size.isValid())
    {
        QTimer::singleShot(200, [=](){resize(window_size);});  // todo: remove this magic
    }
    ui->setupUi(this);
    setWindowTitle("Auto Saver");
    const QString filename = settings->value("filename").toString();
    ui->source_file_path->setText(filename);
    const QString dirname = settings->value("dir").toString();
    ui->destination_dir_path->setText(dirname);
    connect(ui->choose_file, &QPushButton::clicked, this,
            [&](){
                QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Save (*.*)"));
                ui->source_file_path->setText(filename);
                settings->setValue("filename", filename);
                if (ui->destination_dir_path->text().isEmpty()) {
                    ui->destination_dir_path->setText(filename + "-backup/");
                }

                fill_table();
            });
    connect(ui->choose_directory, &QPushButton::clicked, this,
            [&](){
                QString dir = QFileDialog::getExistingDirectory(this,
                                                                tr("Open Directory"),
                                                                "",
                                                                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
                settings->setValue("dir", dir);
                ui->destination_dir_path->setText(dir);

                fill_table();
            });
    fill_table();

    changes_timer->setSingleShot(true);
    changes_timer->setInterval(1000);
    connect(changes_timer, &QTimer::timeout, this, &MainWindow::check_changes);

    writing_timer->setSingleShot(true);
    changes_timer->setInterval(2000);
    connect(writing_timer, &QTimer::timeout, this, &MainWindow::check_writing);
}

MainWindow::~MainWindow()
{
    settings->setValue("window_size", window_size);  // todo: settings is destroyed here (invalid)
    settings->setValue("column_widths", column_widths);
    delete ui;
    ui = nullptr;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
   window_size = event->size();
}

void MainWindow::fill_table()
{
    if (ui->source_file_path->text().isEmpty() || ui->destination_dir_path->text().isEmpty())
    {
        return;
    }
    ui->table->clear();

    ui->table->setColumnCount(ColumnSize);
    QTableWidgetItem *date_item = new QTableWidgetItem("Date");
    ui->table->setHorizontalHeaderItem(DateColumn, date_item);
    QTableWidgetItem *name_item = new QTableWidgetItem("Name");
    ui->table->setHorizontalHeaderItem(NameColumn, name_item);
    QTableWidgetItem *apply_item = new QTableWidgetItem("Apply save");
    ui->table->setHorizontalHeaderItem(ApplyColumn, apply_item);

    const QString saved_file = ui->source_file_path->text().section('/', -1, -1);
    QDir dir(ui->destination_dir_path->text());
    const QStringList sorted_filenames = dir.entryList(QDir::Files, QDir::Name);
    for (const QString& filename : sorted_filenames)
    {
        add_file_to_table(ui->destination_dir_path->text(), filename);
    }
    comare_source_with_table();
    check_changes();
}

void MainWindow::add_file_to_table(const QString& dir_path, const QString& file_name)
{
    const QString saved_file = ui->source_file_path->text().section('/', -1, -1);
    const int i = file_name.lastIndexOf(saved_file);
    if (i > 1)
    {
        const QString timestamp_and_name = file_name.left(i - 1);
        if (timestamp_and_name.count('.') > 0)
        {
            QString timestamp_str = timestamp_and_name.left(timestamp_and_name.indexOf('.'));
            QString user_name = timestamp_and_name.right(timestamp_and_name.size() - timestamp_and_name.lastIndexOf('.') - 1);
            QDateTime timestamp = QDateTime::fromString(timestamp_str, date_time_format);
            if (timestamp.isValid())
            {
                const int row = ui->table->rowCount();

                ui->table->insertRow(row);
                QTableWidgetItem *date_item = new QTableWidgetItem(timestamp_str);
                ui->table->setItem(row, DateColumn, date_item);

                Qt::ItemFlags f = date_item->flags();
                f &= ~Qt::ItemIsEditable;
                date_item->setFlags(f);
                QTableWidgetItem *name_item = new QTableWidgetItem(user_name);
                ui->table->setItem(row, NameColumn, name_item);

                QPushButton* apply = new ApplyButton(dir_path + QDir::separator() + file_name, timestamp);  // Don't need to set parent here.
                ui->table->setCellWidget(row, ApplyColumn, apply);
                connect(apply, &QPushButton::clicked, this, &MainWindow::apply_save_file);
            }
        }
    }
}

void MainWindow::check_changes()
{
    const QDateTime new_last_modifed = modifed_time();
    if (new_last_modifed.isValid() && new_last_modifed > last_modified)
    {
        last_modified = new_last_modifed;
        writing_timer->start();
    }
    else
    {
        changes_timer->start();
    }
}

void MainWindow::check_writing()
{
    const QDateTime new_last_modifed = modifed_time();
    if (new_last_modifed == last_modified)
    {
        last_modified = new_last_modifed;
        const QString destination_file = new_last_modifed.toString(date_time_format) + ".." + ui->source_file_path->text().section('/', -1, -1);
        const QString destination_path(ui->destination_dir_path->text() + QDir::separator() + destination_file);
        const QString source_path = ui->source_file_path->text();
        if (!QFile::exists(destination_path))
        {
            if (QFile::copy(source_path, destination_path))
            {
                add_file_to_table(ui->destination_dir_path->text(), destination_file);
            }
            else
            {
                error(QString("Can not copy '%1' to '%2'. Check either rights and existence of file.").arg(source_path, destination_path));
            }
        }
        changes_timer->start();
    }
    else if (new_last_modifed.isValid())
    {
        writing_timer->start();
    }
    else
    {
        changes_timer->start();
    }
}

void MainWindow::apply_save_file()
{
    changes_timer->stop();
    writing_timer->stop();

    QObject* o = sender();
    ApplyButton* apply = qobject_cast<ApplyButton*>(o);
    assert(apply);
    const QString source = apply->saved_file_path;
    const QString destination = ui->source_file_path->text();
    if (QFile::exists(destination))
    {
        if (!QFile::remove(destination))
        {
           error(QString("Can not remove '%1'. Check either rights and existence of file.").arg(destination));
        }
    }
    if (!QFile::exists(destination))
    {
        if (QFile::copy(source, destination))
        {
            comare_source_with_table();
        }
        else
        {
            error(QString("Can not copy '%1' to '%2'. Check either rights and existence of file.").arg(source, destination));
        }
    }

    changes_timer->start();
}

void MainWindow::error(const QString &text)
{
    QMessageBox::critical(this, "ERROR", text);
}

void MainWindow::comare_source_with_table()
{
    QDateTime modified = QDateTime::fromSecsSinceEpoch(modifed_time().toSecsSinceEpoch());
    if (modified.isValid())
    {
        for (int row = 0; row < ui->table->rowCount(); ++ row)
        {
            ApplyButton* apply = qobject_cast<ApplyButton*>(ui->table->cellWidget(row, ApplyColumn));
            assert(apply);
            std::cout << modified.toString(date_time_format).toLocal8Bit().data() << " "
                      << row  << " "
                      << apply->timestamp.toString(date_time_format).toLocal8Bit().data() << std::endl;
            const bool same_file = modified == apply->timestamp;
            apply->setText(same_file ? "Applied" : "Apply");
            apply->setEnabled(!same_file);
        }
    }
}

QDateTime MainWindow::modifed_time()
{
    const QFileInfo info(ui->source_file_path->text());
    if (!info.isFile())
    {
        error(QString("File '%1' deleted or not regular file").arg(ui->source_file_path->text()));
        return QDateTime();
    }
    return info.lastModified();
}
