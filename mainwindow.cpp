#include "mainwindow.h"
#include "./ui_mainwindow.h"

//#include <iostream>

#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegExp>
#include <QResizeEvent>
#include <QSettings>
#include <QTimer>

#include "applybutton.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settings(new QSettings("auto-saver.ini", QSettings::IniFormat, this))
    , last_modified(QDateTime::fromMSecsSinceEpoch(0))
    , changes_timer(new QTimer(this))
    , writing_timer(new QTimer(this))
    , separator("(\\\\|/)+")
{
    ui->setupUi(this);
    ui->error->setVisible(false);
    setWindowTitle("Auto Saver");

    const QString filename = settings->value("file").toString();
    ui->source_file_path->setText(filename);
    connect(ui->choose_file, &QPushButton::clicked, this, &MainWindow::choose_file);

    const QString dirname = settings->value("dir").toString();
    ui->destination_dir_path->setText(dirname);
    connect(ui->choose_directory, &QPushButton::clicked, this, &MainWindow::choose_dir);
//    connect(ui->destination_dir_path, &QLabel::textChanged, this, &MainWindow::choose_dir);

    connect(ui->table, &QTableWidget::itemChanged, this, &MainWindow::rename);
    fill_table();

    changes_timer->setSingleShot(true);
    changes_timer->setInterval(1000);
    changes_timer->stop();
    connect(changes_timer, &QTimer::timeout, this, &MainWindow::check_changes);

    writing_timer->setSingleShot(true);
    changes_timer->setInterval(2000);
    changes_timer->stop();
    connect(writing_timer, &QTimer::timeout, this, &MainWindow::check_writing);
}

MainWindow::~MainWindow()
{
    settings->setValue("dir", ui->destination_dir_path->text());
    settings->setValue("file", ui->source_file_path->text());
    if (window_size.isValid()) {
        settings->setValue("window_size", window_size);
    }
    QList<QVariant> column_widths;
    for (int column = 0; column < ui->table->columnCount(); ++column) {
        column_widths.append(ui->table->columnWidth(column));
    }
    if (column_widths.size() == ColumnSize) {
        settings->setValue("column_widths", column_widths);
    }

    delete ui;
    ui = nullptr;
}


void MainWindow::choose_file()
{
    QString dir = get_dir_from_paths({ui->source_file_path->text(), ui->destination_dir_path->text()});
    QString source_file_path = QFileDialog::getOpenFileName(this,
                                                            tr("Open File"),
                                                            dir,
                                                            tr("Save (*.*)"));
    if (!source_file_path.isEmpty()) {
        ui->source_file_path->setText(source_file_path);
        if (ui->destination_dir_path->text().isEmpty()) {
            const QString source_dir = source_file_path.section(separator, 0, -2);
            const QString source_file = source_file_path.section(separator, -1);
            const QString new_dir = source_file + " - backup";
            const QString new_dir_path = source_dir + QDir::separator() + new_dir;
            ui->destination_dir_path->setText(new_dir_path);
            QDir dir(new_dir_path);
            if (!dir.exists()) {
                QDir(source_dir).mkdir(new_dir);
            }
        }
        fill_table();
    }
}

void MainWindow::choose_dir()
{
    QString dir = get_dir_from_paths({ui->destination_dir_path->text(), ui->source_file_path->text()});
    QString destination_dir_path = QFileDialog::getExistingDirectory(this,
                                                                     tr("Open Directory"),
                                                                     dir,
                                                                     QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!destination_dir_path.isEmpty()) {
        ui->destination_dir_path->setText(destination_dir_path);
        fill_table();
    }
}

QString MainWindow::get_dir_from_paths(const QStringList &list)
{
    for (const QString& path : list) {
        if (!path.isEmpty()) {
            QString dir = path.section(separator, 0, -2);
            return dir;
        }
    }
    return QString();
}
void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (painting_started) {
        window_size = event->size();
    }
    QMainWindow::resizeEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    if (!painting_started)
    {
        window_size = settings->value("window_size").toSize();
        if (window_size.isValid())
            QTimer::singleShot(0,
                               [=](){
                                    resize(window_size);
                               });
    }
    painting_started = true;
}

void MainWindow::fill_table()
{
    if (ui->source_file_path->text().isEmpty() || ui->destination_dir_path->text().isEmpty())
    {
        return;
    }

    changes_timer->stop();
    writing_timer->stop();

    ui->table->clear();
    saved_files.clear();

    ui->table->setColumnCount(ColumnSize);
    QTableWidgetItem *date_item = new QTableWidgetItem("Date");
    ui->table->setHorizontalHeaderItem(DateColumn, date_item);
    QTableWidgetItem *name_item = new QTableWidgetItem("Name");
    ui->table->setHorizontalHeaderItem(NameColumn, name_item);
    QTableWidgetItem *apply_item = new QTableWidgetItem("Apply save");
    ui->table->setHorizontalHeaderItem(ApplyColumn, apply_item);

    QList<QVariant> column_widths = settings->value("column_widths").toList();
    if (column_widths.size() == ColumnSize) {
        for (int column = 0; column < ui->table->columnCount(); ++column) {
            int width = column_widths[column].toInt();
            if (width) {
                ui->table->setColumnWidth(column, width);
            }
        }
    }

    const QString source_file = ui->source_file_path->text().section(separator, -1, -1);
    QDir dir(ui->destination_dir_path->text());
    const QStringList sorted_filenames = dir.entryList(QDir::Files, QDir::Name);
    for (const QString& filename : sorted_filenames)
    {
        add_file_to_table(ui->destination_dir_path->text(), filename);
    }
    compare_source_with_table();
    ui->table->scrollToBottom();
    check_changes();
}

void MainWindow::add_file_to_table(const QString& dir_path, const QString& file_name, int insert_row)
{
    const QString source_file = ui->source_file_path->text().section(separator, -1, -1);
    const int i = file_name.lastIndexOf(source_file);
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
                assert(insert_row <= ui->table->rowCount());
                const int row = insert_row >= 0 ? insert_row : ui->table->rowCount();

                ui->table->insertRow(row);
                SavedFile sf{dir_path + QDir::separator() + file_name, timestamp};
                saved_files.insert(row, sf);

                QTableWidgetItem *date_item = new QTableWidgetItem(timestamp_str);
                ui->table->setItem(row, DateColumn, date_item);

                Qt::ItemFlags f = date_item->flags();
                f &= ~Qt::ItemIsEditable;
                date_item->setFlags(f);
                QTableWidgetItem *name_item = new QTableWidgetItem(user_name);
                ui->table->setItem(row, NameColumn, name_item);

                QPushButton* apply = new ApplyButton(row);  // Don't need to set parent here.
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
        QString source_file = ui->source_file_path->text().section(separator, -1, -1);
        const QString destination_file = new_last_modifed.toString(date_time_format) + ".." + source_file;
        const QString destination_path(ui->destination_dir_path->text() + QDir::separator() + destination_file);
        const QString source_path = ui->source_file_path->text();
        if (!QFile::exists(destination_path))
        {
            if (QFile::copy(source_path, destination_path))
            {
                log(QString("Copied '%1' to '%2'").arg(source_path, destination_path));
                ui->error->setVisible(false);
                add_file_to_table(ui->destination_dir_path->text(), destination_file);
                compare_source_with_table();
            }
            else
            {
                error(QString("Can not copy '%1' to '%2'. Check either rights and existence of file.").arg(source_path, destination_path));
                ui->error->setVisible(true);
                last_modified = QDateTime::fromSecsSinceEpoch(0);
                return;
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
    const QString source = saved_files[apply->row].file_path;
    const QString destination = ui->source_file_path->text();
    if (QFile::exists(destination))
    {
        if (QFile::remove(destination))
        {
            log(QString("Removed '%1'").arg(destination));
        }
        else
        {
           error(QString("Can not remove '%1'. Check either rights and existence of file.").arg(destination));
        }
    }
    if (!QFile::exists(destination))
    {
        if (QFile::copy(source, destination))
        {
            log(QString("Copied '%1' to '%2'").arg(source, destination));
            compare_source_with_table();
        }
        else
        {
            error(QString("Can not copy '%1' to '%2'. Check either rights and existence of file.").arg(source, destination));
        }
    }
    ui->table->selectRow(apply->row);
    changes_timer->start();
}

void MainWindow::error(const QString &text)
{
    QMessageBox::critical(this, "ERROR", text);
}

void MainWindow::log(const QString &text)
{
//    std::cout << (QDateTime::currentDateTime().toString(date_time_format) + ": " + text).toLocal8Bit().data() << std::endl;
}

void MainWindow::compare_source_with_table()
{
    QDateTime curren_modified = modifed_time();
    for (int row = 0; row < ui->table->rowCount(); ++ row)
    {
        ApplyButton* apply = qobject_cast<ApplyButton*>(ui->table->cellWidget(row, ApplyColumn));
        assert(apply);
        const bool same_file = curren_modified == saved_files[row].timestamp;
        apply->setText(same_file ? "Applied" : "Apply");
        apply->setEnabled(!same_file);
    }
}

void MainWindow::compare_source_with_table(int row)
{
    ApplyButton* apply = qobject_cast<ApplyButton*>(ui->table->cellWidget(row, ApplyColumn));
    assert(apply);
    const bool same_file = last_modified == saved_files[row].timestamp;
    apply->setText(same_file ? "Applied" : "Apply");
    apply->setEnabled(!same_file);
}

void MainWindow::rename(QTableWidgetItem *item)
{
    if (item->column() == NameColumn) {
        int row = item->row();
        ApplyButton* apply = qobject_cast<ApplyButton*>(ui->table->cellWidget(row, ApplyColumn));
        if (!apply) {
            return; // todo inspect
        }
        QString destination_file_name = ui->table->item(row, DateColumn)->text() + "."
                            + ui->table->item(row, NameColumn)->text() + "." + ui->source_file_path->text().section(separator, -1, -1);
        QString destination_path = ui->destination_dir_path->text() + QDir::separator() + destination_file_name;
        QString source_path = saved_files[row].file_path;
        if (source_path != destination_path) {
            if (QFile::rename(source_path, destination_path)) {
                log(QString("Renamed '%1' to '%2'").arg(source_path, destination_path));
                saved_files[row].file_path = destination_path;
            }
            else {
                error(QString("Can not rename '%1' to '%2'. Don't type forbidden symbol or check either right and existance of files.").arg(source_path, destination_path));

                // Block recursion
                ui->table->blockSignals(true);
                item->setText("");
                ui->table->blockSignals(false);

                return;
            }
            compare_source_with_table();
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
    QDateTime without_ms = QDateTime::fromSecsSinceEpoch(info.lastModified().toSecsSinceEpoch());
    return without_ms;
}
