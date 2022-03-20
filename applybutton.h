#pragma once

#include <QPushButton>
#include <QDateTime>


class ApplyButton : public QPushButton
{
    Q_OBJECT
public:
    const int row;

    explicit ApplyButton(const int row_, QWidget *parent = nullptr);
};
