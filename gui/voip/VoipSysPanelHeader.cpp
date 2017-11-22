#include "stdafx.h"
#include "VoipSysPanelHeader.h"

#include "VoipTools.h"
#include "../utils/utils.h"
#include "../../installer/utils/styles.h"

#ifdef __APPLE__
#   include <QtWidgets/qtoolbutton.h>
#else
#   include <QToolButton>
#   include <QStyleOptionToolButton>
#endif // __APPLE__

#define VOIP_VIDEO_PANEL_BTN_OFFSET (Utils::scale_value(40))

Ui::IncomingCallControls::IncomingCallControls(QWidget* _parent)
    : BaseBottomVideoPanel(_parent)
    , parent_(_parent)
    , rootWidget_(nullptr)
{
#ifndef __linux__
    setStyleSheet(Utils::LoadStyle(":/qss/incoming_call"));
#else
    setStyleSheet(Utils::LoadStyle(":/qss/incoming_call_linux"));
#endif

    setProperty("IncomingCallControls", true);

#ifndef __linux__
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

    rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setProperty("IncomingCallControls", true);
    layout()->addWidget(rootWidget_);

    QHBoxLayout* layoutTarget = Utils::emptyHLayout();
    layoutTarget->setAlignment(Qt::AlignVCenter);
    rootWidget_->setLayout(layoutTarget);

    QWidget* rootWidget = rootWidget_;
    const int btnMinWidth = Utils::scale_value(40);

    auto getWidget = [rootWidget] (const bool _vertical)
    {
        QBoxLayout* layoutInWidget;
        if (_vertical)
        {
            layoutInWidget = Utils::emptyVLayout();
        }
        else
        {
            layoutInWidget = Utils::emptyHLayout();
        }

        layoutInWidget->setAlignment(Qt::AlignCenter);

        QWidget* widget = new voipTools::BoundBox<QWidget>(rootWidget);
        widget->setContentsMargins(0, 0, 0, 0);
        widget->setLayout(layoutInWidget);

        return widget;
    };

    auto getButton = [this, btnMinWidth] (QWidget* _parent, const char* _propertyName, const char* /*text*/, const char* _slot, QPushButton** _btnOut)->int
    {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(_parent);
        btn->setProperty(_propertyName, true);
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        connect(btn, SIGNAL(clicked()), this, _slot, Qt::QueuedConnection);
        *_btnOut = btn;

        btn->resize(btn->sizeHint().width(), btn->sizeHint().height());
        return (btn->width() - btnMinWidth) / 2;
    };

    auto getLabel = [this, btnMinWidth] (QWidget* _parent, const char* /*_propertyName*/, const char* _text, QLabel** _labelOut)->int
    {
        QLabel* label = new voipTools::BoundBox<QLabel>(_parent);
        label->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
        label->setText(_text);
        label->setFont(Fonts::appFontScaled(12));
        label->setAlignment(Qt::AlignCenter);
        label->setProperty("VoipPanel_Has_Video", false);

        *_labelOut = label;

        label->resize(label->sizeHint().width(), label->sizeHint().height());
        return (label->width() - btnMinWidth) * 0.5f;
    };

    QWidget* widget1 = getWidget(true);
    QWidget* widget2 = getWidget(true);
    QWidget* widget3 = getWidget(true);

    int tail1 = 0;
    int tail2 = 0;
    int tail3 = 0;

    const struct ButtonDesc
    {
        QWidget*    parent;
        std::string propertyName;
        std::string text;
        std::string slot;
        int&        tail;

        ButtonDesc(QWidget*    parent,
            std::string propertyName,
            std::string text,
            std::string slot,
            int&        tail) : parent(parent), propertyName(propertyName), text(text), slot(slot), tail(tail) {}
    }
    buttonsToCreate [] =
    {
        ButtonDesc(
            widget1,
            "IncomingCall_hangup",
            QT_TRANSLATE_NOOP("voip_pages","End call").toUtf8().data(),
            SLOT(_onDecline()),
            tail1
        ),
        ButtonDesc(
            widget2,
            "IncomingCall_audio",
            QT_TRANSLATE_NOOP("voip_pages", "Answer").toUtf8().data(),
            SLOT(_onAudio()),
            tail2
        ),
        ButtonDesc(
            widget3,
            "IncomingCall_video",
            QT_TRANSLATE_NOOP("voip_pages", "Video").toUtf8().data(),
            SLOT(_onVideo()),
            tail3
        )
    };

    for (unsigned ix = 0; ix < sizeof buttonsToCreate / sizeof buttonsToCreate[0]; ++ix)
    {
        const ButtonDesc& buttonDesc = buttonsToCreate[ix];

        QPushButton* btn = NULL;
        const int btnTail = getButton(buttonDesc.parent, buttonDesc.propertyName.c_str(), buttonDesc.text.c_str(), buttonDesc.slot.c_str(), &btn);

        QLabel* label = NULL;
        const int labelTail = getLabel(buttonDesc.parent, buttonDesc.propertyName.c_str(), buttonDesc.text.c_str(), &label);

        assert(btn && label);
        if (!btn || !label)
        {
            continue;
        }
        buttonDesc.tail = std::max(btnTail, labelTail);

        QWidget* tmpWidget = getWidget(false);
        tmpWidget->layout()->addWidget(btn);
        buttonDesc.parent->layout()->addWidget(tmpWidget);
        buttonDesc.parent->layout()->addWidget(label);
    }

    layoutTarget->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    layoutTarget->addWidget(widget1);
    layoutTarget->addSpacing(VOIP_VIDEO_PANEL_BTN_OFFSET - tail1 - tail2);
    layoutTarget->addWidget(widget2);
    layoutTarget->addSpacing(VOIP_VIDEO_PANEL_BTN_OFFSET - tail2 - tail3);
    layoutTarget->addWidget(widget3);
    layoutTarget->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
}

Ui::IncomingCallControls::~IncomingCallControls()
{
}

void Ui::IncomingCallControls::_onDecline()
{
    emit onDecline();
}

void Ui::IncomingCallControls::_onAudio()
{
    emit onAudio();
}

void Ui::IncomingCallControls::_onVideo()
{
    emit onVideo();
}

void Ui::IncomingCallControls::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
    if (_e->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow() || rootWidget_->isActiveWindow())
        {
            if (parent_)
            {
                //QSignalBlocker sb(parent_);
                parent_->raise();
                raise();
            }
        }
    }
}

void Ui::IncomingCallControls::setVideoStatus(bool video)
{
    QList<QLabel *> underButtonText = rootWidget_->findChildren<QLabel *>();

    std::for_each(underButtonText.begin(), underButtonText.end(), [=] (QLabel * lable) {
        lable->setProperty("VoipPanel_Has_Video", video);
        lable->style()->unpolish(lable);
        lable->style()->polish(lable);
        lable->update();
        });
}


Ui::VoipSysPanelHeader::VoipSysPanelHeader(QWidget* _parent)
    : MoveablePanel(_parent)
    , avatarContainer_(nullptr)
    , nameAndStatusContainer_(nullptr)
    , rootWidget_(nullptr)
{
#ifdef __linux__
    setStyleSheet(Utils::LoadStyle(":/qss/video_panel_linux"));
#else
    setStyleSheet(Utils::LoadStyle(":/qss/video_panel"));
#endif

#ifdef __APPLE__
    setProperty("VoipPanelHeaderMac", true);
#else
    setProperty("VoipPanelHeader", true);
#endif

    //setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    //setAttribute(Qt::WA_NoSystemBackground, true);
    //setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

    rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setObjectName("VoipPanelHeader");
    rootWidget_->setProperty("VoipPanelHeaderText_Has_Video", false);
    layout()->addWidget(rootWidget_);

    QHBoxLayout* layout = Utils::emptyHLayout();
    layout->setAlignment(Qt::AlignVCenter);
    rootWidget_->setLayout(layout);

    avatarContainer_ = new voipTools::BoundBox<AvatarContainerWidget>(rootWidget_, Utils::scale_value(66), Utils::scale_value(12), Utils::scale_value(12));
    avatarContainer_->setOverlap(0.2f);
    rootWidget_->layout()->addWidget(avatarContainer_);

    nameAndStatusContainer_ = new NameAndStatusWidget(rootWidget_, Utils::scale_value(18), Utils::scale_value(15));
    nameAndStatusContainer_->setNameProperty("VoipPanelHeaderText_Name", true);
    nameAndStatusContainer_->setNameProperty("VoipPanelHeaderText_Has_Video", false);

    nameAndStatusContainer_->setStatusProperty("VoipPanelHeaderText_Status", true);
    nameAndStatusContainer_->setStatusProperty("VoipPanelHeaderText_Has_Video", false);
    nameAndStatusContainer_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    rootWidget_->layout()->addWidget(nameAndStatusContainer_);

    layout->addSpacing(Utils::scale_value(12));
}

Ui::VoipSysPanelHeader::~VoipSysPanelHeader()
{

}

void Ui::VoipSysPanelHeader::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    emit onMouseEnter();
}

void Ui::VoipSysPanelHeader::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    emit onMouseLeave();
}

void Ui::VoipSysPanelHeader::setAvatars(const std::vector<std::string> &avatarList)
{
    if (avatarContainer_)
    {
        avatarContainer_->dropExcess(avatarList);
    }
}

void Ui::VoipSysPanelHeader::setTitle(const char* _s)
{
    if (nameAndStatusContainer_)
    {
        nameAndStatusContainer_->setName(_s);
    }
}

void Ui::VoipSysPanelHeader::setStatus(const char* _s)
{
    if (nameAndStatusContainer_)
    {
        nameAndStatusContainer_->setStatus(_s);
    }
}

bool Ui::VoipSysPanelHeader::uiWidgetIsActive() const
{
    if (rootWidget_)
    {
        return rootWidget_->isActiveWindow();
    }
    return false;
}

void Ui::VoipSysPanelHeader::setVideoStatus(bool video)
{
    if (nameAndStatusContainer_)
    {
        nameAndStatusContainer_->setStatusProperty("VoipPanelHeaderText_Has_Video", video);
        nameAndStatusContainer_->setNameProperty("VoipPanelHeaderText_Has_Video", video);
    }
    rootWidget_->setProperty("VoipPanelHeaderText_Has_Video", video);
    rootWidget_->style()->unpolish(rootWidget_);
    rootWidget_->style()->polish(rootWidget_);
}

void Ui::VoipSysPanelHeader::fadeIn(unsigned int duration) {}

void Ui::VoipSysPanelHeader::fadeOut(unsigned int duration) {}



