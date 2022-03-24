#pragma once

#include <QMainWindow>
#include <QDateTime>
#include <QTableWidgetItem>

#include "savefile.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum {
        DateColumn,
        TurnColumn,
        NameColumn,
        ApplyColumn,
        ColumnSize,
    };

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
private:
    Ui::MainWindow *ui = nullptr;
    class QSettings* settings;
    QDateTime last_modified;
    bool painting_started = false;
    QSize window_size;
    class QTimer* changes_timer = nullptr;
    class QTimer* writing_timer = nullptr;
    QList<SavedFile> saved_files;
    const QRegExp separator;

    void fill_table();
    void add_file_to_table(const QString& file_name);
    QDateTime modifed_time();
    void log(const QString &text);
    QString get_dir_from_paths(const QStringList& list);
    void apply_save_file();
    void error(const QString& text);
    void compare_source_with_table();
    void rename(QTableWidgetItem *item);
private slots:
    void choose_file();
    void choose_dir();
    void check_changes();
    void check_writing();
};
