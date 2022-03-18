#pragma once

#include <QMainWindow>
#include <QDateTime>

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

private:
    Ui::MainWindow *ui = nullptr;
    class QSettings* settings = nullptr;
    QDateTime last_modified;
    void fill_table();
    class QTimer* changes_timer = nullptr;
    class QTimer* writing_timer = nullptr;
    void add_file_to_table(const QString& dir_path, const QString& file_name);
private slots:
    void check_changes();
    void check_writing();
    void apply_save_file();
};
