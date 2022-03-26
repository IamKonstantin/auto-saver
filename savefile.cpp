#include "savefile.h"

#include <QDir>
#include <QDateTime>
#include <QRegExp>

//#include <iostream>


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
        QRegExp valid_prefix(R"REGEXPR((^\d{4}-\d{2}-\d{2} \d{2}-\d{2}-\d{2})\.(.*)$)REGEXPR");
        if (valid_prefix.exactMatch(prefix))
        {
            QString timestamp_str = valid_prefix.cap(1);
            QString turn_str = valid_prefix.cap(2);
            name = valid_prefix.cap(2);
            timestamp = QDateTime::fromString(timestamp_str, date_time_format);
        }
    }
    turn = read_turn();
}

QString SavedFile::get_file_path() const
{
    return destination_dir_path + QDir::separator() + get_file_name();
}

QString SavedFile::get_file_name() const
{
    return timestamp.toString(date_time_format) + '.' + name + '.' + source_file;
}

int SavedFile::read_turn()
{
    QFile file(get_file_path());
    if (!file.open(QIODevice::ReadOnly))
    {
        return 0;
    }

    constexpr int NEEDED_BYTES = 103;
    constexpr int MAX_DATA_OFFSET = 18;

    QByteArray data = file.read(NEEDED_BYTES);
    if (data.size() < NEEDED_BYTES)
    {
        return 0;
    }
//    for (int i = 0; i < data.size(); ++ i)
//    {
//        std::cout << i << " " << int(data[i]) << std::endl;
//    }

    auto get_turn = [&](int offset)->int {
        char prefix_0 = data[offset];
        char turn_0 = data[offset + 1];

        char prefix_1 = data[offset + 2];
        char turn_1 = data[offset + 3];

        char prefix_2 = data[offset + 17];
        char turn_2 = data[offset + MAX_DATA_OFFSET];

        if (prefix_0 == 0x16
                && prefix_1 == 0x16
                && prefix_2 == 0x16
                && turn_0 == turn_1
                && turn_1 == turn_2)
        {
            return turn_0;
        }
        else
        {
            return 1;  // valid
        }
    };

    constexpr int WARHAMMER_2_OFFSET = 84;
    static_assert(NEEDED_BYTES > WARHAMMER_2_OFFSET + MAX_DATA_OFFSET, "");
    int w2 = get_turn(WARHAMMER_2_OFFSET);
    if (w2 > 1)
    {
        return w2;
    }
    else
    {
        constexpr int WARHAMMER_3_OFFSET = 63;
        static_assert(NEEDED_BYTES > WARHAMMER_3_OFFSET + MAX_DATA_OFFSET, "");
        return get_turn(WARHAMMER_3_OFFSET);
    }
}
