#include "stdafx.h"
#include "BackButton.h"
#include "../utils/utils.h"

namespace Ui
{
    BackButton::BackButton(QWidget* _parent)
        : QPushButton(_parent)
    {
        setStyleSheet(Utils::LoadStyle(qsl(":/qss/controls_style")));
        setObjectName(qsl("backButton"));
    }
    BackButton::~BackButton()
    {
        //
    }
}
