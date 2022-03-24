#pragma once

#include <QString>
#include <QDateTime>


struct SavedFile {
    // Does QString has build-in referencing to reduce coping of same string?
    const QString source_file_path;
    const QString source_file;
    const QString destination_dir_path;

    QDateTime timestamp;
    int turn = 0;
    QString name;

    SavedFile(const QString& source_file_path_, const QString& destination_dir_path_, const QDateTime& timestamp_);
    SavedFile(const QString& source_file_path_, const QString& destination_dir_path_, const QString& existing_file_name);

    bool is_valid() {
        return timestamp.isValid();
    }

    QString get_file_path();
    QString get_file_name();
};
