#include "applybutton.h"

ApplyButton::ApplyButton(const QString &saved_file_path_, QWidget *parent)
    : QPushButton{parent}
    , saved_file_path(saved_file_path_)
{

}
