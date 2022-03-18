#pragma once

#include <QMainWindow>
#include <QDateTime>
#include <QPointer>
#include <QSettings>


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
private:
    Ui::MainWindow *ui = nullptr;
    QPointer<QSettings> settings;
    QDateTime last_modified;
    QSize window_size;
    QList<QVariant> column_widths;
    class QTimer* changes_timer = nullptr;
    class QTimer* writing_timer = nullptr;

    void fill_table();
    void add_file_to_table(const QString& dir_path, const QString& file_name);
private slots:
    void check_changes();
    void check_writing();
    void apply_save_file();
    void error(const QString& text);
    void comare_source_with_table();
    QDateTime modifed_time();
};

