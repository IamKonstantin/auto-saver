#include "savefile.h"

#include <QDir>
#include <QDateTime>
#include <QRegExp>


// I don't like next two lines
const QRegExp dir_separator("(\\\\|/)+");
constexpr auto date_time_format = "yyyy-MM-dd hh-mm-ss";


SavedFile::SavedFile(const QString &source_file_path_, const QString &destination_dir_path_, const QDateTime &timestamp_)
    : source_file_path(source_file_path_)
    , source_file(source_file_path_.section(dir_separator, -1, -1))
    , destination_dir_path(destination_dir_path_)
    , timestamp(timestamp_)
{

}

SavedFile::SavedFile(const QString &source_file_path_, const QString& destination_dir_path_, const QString &existing_file_name)
    : source_file_path(source_file_path_)
    , source_file(source_file_path_.section(dir_separator, -1, -1))
    , destination_dir_path(destination_dir_path_)
{
    const int i = existing_file_name.lastIndexOf(source_file);
    if (i > 1)
    {
        const QString prefix = existing_file_name.left(i - 1);
        QRegExp valid_prefix(R"REGEXPR((^\d{4}-\d{2}-\d{2} \d{2}-\d{2}-\d{2})\.turn(\d+)\.(.*)$)REGEXPR");
        if (valid_prefix.exactMatch(prefix))
        {
            QString timestamp_str = valid_prefix.cap(1);
            QString turn_str = valid_prefix.cap(2);
            name = valid_prefix.cap(3);
            timestamp = QDateTime::fromString(timestamp_str, date_time_format);
            turn = turn_str.toInt();
        }
    }
}

QString SavedFile::get_file_path()
{
    return destination_dir_path + QDir::separator() + get_file_name();
}

QString SavedFile::get_file_name()
{
    return timestamp.toString(date_time_format) + '.' + "turn" + QString::number(turn) + '.' + name + '.' + source_file;
}


