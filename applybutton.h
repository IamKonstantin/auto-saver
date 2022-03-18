#pragma once

#include <QPushButton>
#include <QDateTime>


class ApplyButton : public QPushButton
{
    Q_OBJECT
public:
    const QString saved_file_path;
    const QDateTime timestamp;

    explicit ApplyButton(const QString& saved_file_path_, const QDateTime& timestamp_, QWidget *parent = nullptr);
};
