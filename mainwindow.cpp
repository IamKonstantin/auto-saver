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
    , changes_timer(new QTimer(this))
    , separator("(\\\\|/)+")
{
    ui->setupUi(this);
    ui->error->setVisible(false);
    setWindowTitle(tr("Auto Saver"));

    const QString filename = settings->value("file").toString();
    ui->source_file_path->setText(filename);
    connect(ui->choose_file, &QPushButton::clicked, this, &MainWindow::choose_file);

    const QString dirname = settings->value("dir").toString();
    ui->destination_dir_path->setText(dirname);
    connect(ui->choose_directory, &QPushButton::clicked, this, &MainWindow::choose_dir);

    ui->table->setColumnCount(ColumnSize);
    QTableWidgetItem *date_item = new QTableWidgetItem(tr("Date"));
    ui->table->setHorizontalHeaderItem(DateColumn, date_item);
    QTableWidgetItem *turn_item = new QTableWidgetItem(tr("Turn"));
    ui->table->setHorizontalHeaderItem(TurnColumn, turn_item);
    QTableWidgetItem *name_item = new QTableWidgetItem(tr("Name"));
    ui->table->setHorizontalHeaderItem(NameColumn, name_item);
    QTableWidgetItem *apply_item = new QTableWidgetItem(tr("Apply save"));
    ui->table->setHorizontalHeaderItem(ApplyColumn, apply_item);
    connect(ui->table, &QTableWidget::itemChanged, this, &MainWindow::rename);

    changes_timer->setSingleShot(true);
    changes_timer->setInterval(1000);
    changes_timer->stop();
    connect(changes_timer, &QTimer::timeout, this, &MainWindow::check_changes);

    fill_table();
}

MainWindow::~MainWindow()
{
    settings->setValue("dir", ui->destination_dir_path->text());
    settings->setValue("file", ui->source_file_path->text());
    if (window_size.isValid())
    {
        settings->setValue("window_size", window_size);
    }
    QList<QVariant> column_widths;
    for (int column = 0; column < ui->table->columnCount(); ++column)
    {
        column_widths.append(ui->table->columnWidth(column));
    }
    if (column_widths.size() == ColumnSize)
    {
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
    const bool user_pressed_ok = !source_file_path.isEmpty();
    const bool file_changed = source_file_path != ui->source_file_path->text();
    if (user_pressed_ok && file_changed)
    {
        source_file_path.replace(separator, QDir::separator());
        ui->source_file_path->setText(source_file_path);

        const QString source_dir = source_file_path.section(QDir::separator(), 0, -2);
        const QString source_file = source_file_path.section(QDir::separator(), -1);
        const QString new_dir = source_file + " - backup";
        const QString new_dir_path = source_dir + QDir::separator() + new_dir;

        QDir dir(new_dir_path);
        if (!dir.exists())
        {
            if (QDir(source_dir).mkdir(new_dir))
            {
                log(QString("made directory '%1'").arg(new_dir_path));
            }
            else
            {
                error(tr("Can't make directory '%1'").arg(new_dir_path));
                return;
            }
        }

        ui->destination_dir_path->setText(new_dir_path);
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
    const bool user_pressed_ok = !destination_dir_path.isEmpty();
    const bool dir_changed = destination_dir_path != ui->destination_dir_path->text();
    if (user_pressed_ok && dir_changed)
    {
        destination_dir_path.replace(separator, QDir::separator());
        ui->destination_dir_path->setText(destination_dir_path);
        fill_table();
    }
}

QString MainWindow::get_dir_from_paths(const QStringList &list)
{
    for (const QString& path : list)
    {
        if (!path.isEmpty())
        {
            QString dir = path.section(separator, 0, -2);
            return dir;
        }
    }
    return QString();
}
void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (painting_started)
    {
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

    last_modified = QDateTime::fromMSecsSinceEpoch(0);

    saved_files.clear();
    ui->table->clearContents();
    ui->table->setRowCount(0);

    QDateTime curren_modified = modifed_time();
    if (curren_modified.isValid())
    {
        ui->error->setVisible(false);
    }
    else
    {
        ui->error->setVisible(true);
        return;
    }

    QList<QVariant> column_widths = settings->value("column_widths").toList();
    if (column_widths.size() == ColumnSize)
    {
        for (int column = 0; column < ui->table->columnCount(); ++column)
        {
            int width = column_widths[column].toInt();
            if (width)
            {
                ui->table->setColumnWidth(column, width);
            }
        }
    }

    QDir dir(ui->destination_dir_path->text());
    const QStringList sorted_filenames = dir.entryList(QDir::Files, QDir::Name);
    for (const QString& filename : sorted_filenames)
    {
        add_file_to_table(filename);
    }
    compare_source_with_table();
    ui->table->scrollToBottom();
    check_changes();
}

void MainWindow::add_file_to_table(const QString& file_name)
{
    SavedFile candidat(ui->source_file_path->text(), ui->destination_dir_path->text(), file_name);
    if (candidat.is_valid())
    {
        const int row = ui->table->rowCount();
        ui->table->insertRow(row);
        saved_files.insert(row, candidat);

        static constexpr auto user_format = "yyyy.MM.dd hh:mm:ss";

        QTableWidgetItem *date_item = new QTableWidgetItem(candidat.timestamp.toString(user_format));
        ui->table->setItem(row, DateColumn, date_item);
        Qt::ItemFlags date_flags = date_item->flags();
        date_flags &= ~Qt::ItemIsEditable;
        date_item->setFlags(date_flags);

        QTableWidgetItem *turn_item = new QTableWidgetItem(QString::number(candidat.turn));
        ui->table->setItem(row, TurnColumn, turn_item);
        Qt::ItemFlags turn_flags = turn_item->flags();
        turn_flags &= ~Qt::ItemIsEditable;
        turn_item->setFlags(turn_flags);

        QTableWidgetItem *name_item = new QTableWidgetItem(candidat.name);
        ui->table->setItem(row, NameColumn, name_item);

        QPushButton* apply = new ApplyButton(row);  // Don't need to set parent here.
        ui->table->setCellWidget(row, ApplyColumn, apply);
        connect(apply, &QPushButton::clicked, this, &MainWindow::apply_save_file);
    }
}

void MainWindow::check_changes()
{
    const QDateTime new_last_modifed = modifed_time();
    if (new_last_modifed.isValid())
    {
        QDateTime now = QDateTime::currentDateTime();
        if (new_last_modifed > last_modified && now.toSecsSinceEpoch() - new_last_modifed.toSecsSinceEpoch() > 10)
        {
            last_modified = new_last_modifed;
            SavedFile candidat(ui->source_file_path->text(), ui->destination_dir_path->text(), last_modified);
            const QString destination_path = candidat.get_file_path();
            const QString source_path = ui->source_file_path->text();
            if (!QFile::exists(destination_path))
            {
                if (QFile::copy(source_path, destination_path))
                {
                    log(QString("Copied '%1' to '%2'").arg(source_path, destination_path));
                    add_file_to_table(candidat.get_file_name());
                    compare_source_with_table();
                }
                else
                {
                    error(tr("Can not copy '%1' to '%2'. Check either rights and existence of file.").arg(source_path, destination_path));
                    ui->error->setVisible(true);
                    last_modified = QDateTime::fromSecsSinceEpoch(0);
                    return;
                }
            }
        }

        changes_timer->start();
    }
    else
    {
        ui->error->setVisible(true);
    }
}

void MainWindow::apply_save_file()
{
    changes_timer->stop();

    QObject* o = sender();
    ApplyButton* apply = qobject_cast<ApplyButton*>(o);
    assert(apply);
    const QString source = saved_files[apply->row].get_file_path();
    const QString destination = ui->source_file_path->text();
    if (QFile::exists(destination))
    {
        if (QFile::remove(destination))
        {
            log(QString("Removed '%1'").arg(destination));
        }
        else
        {
           error(tr("Can not remove '%1'. Check either rights and existence of file.").arg(destination));
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
            error(tr("Can not copy '%1' to '%2'. Check either rights and existence of file.").arg(source, destination));
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
    //std::cout << (QDateTime::currentDateTime().toString(date_time_format) + ": " + text).toLocal8Bit().data() << std::endl;
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
        if (same_file)
        {
            last_modified = saved_files[row].timestamp;
        }
    }
}

void MainWindow::rename(QTableWidgetItem *item)
{
    if (item->column() == NameColumn)
    {
        const int row = item->row();
        const QString new_name = ui->table->item(row, NameColumn)->text();
        if (new_name != saved_files[row].name)
        {
            SavedFile candidat(saved_files[row]);
            candidat.name = new_name;

            QString source_path(saved_files[row].get_file_path());
            QString destination_path(candidat.get_file_path());

            if (QFile::rename(source_path, destination_path))
            {
                log(QString("Renamed '%1' to '%2'").arg(source_path, destination_path));
                saved_files[row].name = candidat.name;  // const objects removed assigment consturctor
            }
            else
            {
                error(tr("Can not rename '%1' to '%2'. Don't type forbidden symbol or check either right and existance of files.").arg(source_path, destination_path));

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
        error(tr("File '%1' deleted or not regular file").arg(ui->source_file_path->text()));
        return QDateTime();
    }
    QDateTime without_ms = QDateTime::fromSecsSinceEpoch(info.lastModified().toSecsSinceEpoch());
    return without_ms;
}
