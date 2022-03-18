#pragma once

#include <QPushButton>

class ApplyButton : public QPushButton
{
    Q_OBJECT
public:
    const QString saved_file_path;

    explicit ApplyButton(const QString& saved_file_path_, QWidget *parent = nullptr);
};
