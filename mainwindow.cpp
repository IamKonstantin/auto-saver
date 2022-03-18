#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <iostream>

#include <QFileDialog>
#include <QRegExp>
#include <QSettings>
#include <QTimer>
#include <QDateTime>

#include "applybutton.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settings(new QSettings("auto-saver.ini", QSettings::IniFormat, this))
    , last_modified(QDateTime::fromMSecsSinceEpoch(0))
    , changes_timer(new QTimer(this))
    , writing_timer(new QTimer(this))
{
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

    ui->table->setColumnCount(ColumnSize);
    QTableWidgetItem *date_item = new QTableWidgetItem("Date");
    ui->table->setHorizontalHeaderItem(DateColumn, date_item);
    QTableWidgetItem *name_item = new QTableWidgetItem("Name");
    ui->table->setHorizontalHeaderItem(NameColumn, name_item);
    QTableWidgetItem *apply_item = new QTableWidgetItem("Apply save");
    ui->table->setHorizontalHeaderItem(ApplyColumn, apply_item);

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
    delete ui;
    ui = nullptr;
}

void MainWindow::fill_table()
{
    if (ui->source_file_path->text().isEmpty() || ui->destination_dir_path->text().isEmpty()) {
        return;
    }
    const QString saved_file = ui->source_file_path->text().section('/', -1, -1);
    QDir dir(ui->destination_dir_path->text());
    const QStringList sorted_filenames = dir.entryList(QDir::Files, QDir::Name);
    for (const QString& filename : sorted_filenames) {
        add_file_to_table(ui->destination_dir_path->text(), filename);
    }
    check_changes();
}

void MainWindow::add_file_to_table(const QString& dir_path, const QString& file_name)
{
    const QString saved_file = ui->source_file_path->text().section('/', -1, -1);
    const int i = file_name.lastIndexOf(saved_file);
    if (i > 1) {
        const QString timestamp_and_name = file_name.left(i - 1);
        if (timestamp_and_name.count('.') > 0) {
            QString timestamp = timestamp_and_name.left(timestamp_and_name.indexOf('.'));
            QString user_name = timestamp_and_name.right(timestamp_and_name.size() - timestamp_and_name.lastIndexOf('.') - 1);
            QDateTime dt = QDateTime::fromString(timestamp, date_time_format);
            if (dt.isValid()) {
                const int row = ui->table->rowCount();

                ui->table->insertRow(row);
                QTableWidgetItem *date_item = new QTableWidgetItem(timestamp);
                ui->table->setItem(row, DateColumn, date_item);

                Qt::ItemFlags f = date_item->flags();
                f &= ~Qt::ItemIsEditable;
                date_item->setFlags(f);
                QTableWidgetItem *name_item = new QTableWidgetItem(user_name);
                ui->table->setItem(row, NameColumn, name_item);

                QPushButton* apply = new ApplyButton(saved_file);  // Don't need to set parent here.
                apply->setText("Apply");  // It is here for the one translation context
                ui->table->setCellWidget(row, ApplyColumn, apply);
                connect(apply, &QPushButton::clicked, this, &MainWindow::apply_save_file);
            }
        }
    }
}

void MainWindow::check_changes()
{
    const QFileInfo info(ui->source_file_path->text());
    const QDateTime new_last_modifed = info.lastModified();
    if (new_last_modifed > last_modified) {
        last_modified = new_last_modifed;
        writing_timer->start();
    } else {
        changes_timer->start();
    }
}

void MainWindow::check_writing()
{
    const QFileInfo info(ui->source_file_path->text());
    const QDateTime new_last_modifed = info.lastModified();
    if (new_last_modifed == last_modified) {
        last_modified = new_last_modifed;
        QFile to_copy(ui->source_file_path->text());
        const QString destination_file = new_last_modifed.toString(date_time_format) + ".." + ui->source_file_path->text().section('/', -1, -1);
        const QString destination_path(ui->destination_dir_path->text() + QDir::separator() + destination_file);
        to_copy.copy(destination_path);
        add_file_to_table(ui->destination_dir_path->text(), destination_file);
        changes_timer->start();
    } else {
        writing_timer->start();
    }
}

void MainWindow::apply_save_file()
{
    changes_timer->stop();
    writing_timer->stop();

    QObject* o = sender();
    ApplyButton* apply = qobject_cast<ApplyButton*>(o);
    assert(apply);
    QFile destination(ui->source_file_path->text());
    //destination.remove();
    QFile source(apply->saved_file_path);
    source.copy(ui->source_file_path->text());

    changes_timer->start();
}
