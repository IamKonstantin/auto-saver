#pragma once

#include <QMainWindow>
#include <QDateTime>
#include <QTableWidgetItem>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum {
        DateColumn,
        NameColumn,
        ApplyColumn,
        ColumnSize,
    };
    static constexpr auto date_time_format = "yyyy-MM-dd hh-mm-ss";

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
    struct SavedFile {
        QString file_path;
        QDateTime timestamp;
    };
    QList<SavedFile> saved_files;
    const QRegExp separator;

    void fill_table();
    void add_file_to_table(const QString& dir_path, const QString& file_name, int insert_row = -1);
    QDateTime modifed_time();
    void log(const QString &text);
private slots:
    void choose_file();
    void choose_dir();
    QString get_dir_from_paths(const QStringList& list);
    void check_changes();
    void check_writing();
    void apply_save_file();
    void error(const QString& text);
    void compare_source_with_table();
    void rename(QTableWidgetItem *item);
};

