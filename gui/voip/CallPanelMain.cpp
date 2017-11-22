#include "stdafx.h"
#include "CallPanelMain.h"

#include "PushButton_t.h"
#include "VideoPanelHeader.h"
#include "VoipTools.h"
#include "../core_dispatcher.h"
#include "../controls/CommonStyle.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/utils.h"
#include "../main_window/contact_list/SelectionContactsForGroupChat.h"
#include "../main_window/contact_list/ContactList.h"
#include "../main_window/contact_list/ChatMembersModel.h"
#include "../main_window/MainPage.h"
#include "../utils/InterConnector.h"
#include "../fonts.h"
#include "../controls/ContextMenu.h"

extern const QString vertSoundBg;
extern const QString horSoundBg;

const QString buttonGoChat =
"QPushButton { min-width: 36dip; max-width: 36dip; min-height: 36dip; max-height: 36dip;"
"border-image: url(:/voip/chat_100); }"
"QPushButton:hover { border-image: url(:/voip/chat_100_hover); }";

const QString buttonCameraEnable =
"QPushButton { min-width: 36dip; max-width: 36dip; min-height: 36dip; max-height: 36dip;"
"border-image: url(:/voip/camera_100); }"
"QPushButton:hover { border-image: url(:/voip/camera_100_hover); }";

const QString buttonCameraDisable =
"QPushButton { min-width: 36dip; max-width: 36dip; min-height: 36dip; max-height: 36dip;"
"border-image: url(:/voip/camera_off_100); }"
"QPushButton:hover { border-image: url(:/voip/camera_off_100_hover); }";

const QString buttonStopCall =
"QPushButton { min-height: 36dip; max-height: 36dip; min-width: 36dip; max-width: 36dip;"
"border-image: url(:/voip/endcall_100); }"
"QPushButton:hover { border-image: url(:/voip/endcall_100_hover); }";

const QString buttonMicEnable =
"QPushButton { min-width: 36dip; max-width: 36dip; min-height: 36dip; max-height: 36dip;"
"border-image: url(:/voip/microphone_100); }"
"QPushButton:hover { border-image: url(:/voip/microphone_100_hover); }";

const QString buttonMicDisable =
"QPushButton { min-width: 36dip; max-width: 36dip; min-height: 36dip; max-height: 36dip;"
"border-image: url(:/voip/microphone_off_100); }"
"QPushButton:hover { border-image: url(:/voip/microphone_off_100_hover); }";

const QString buttonSoundEnable =
"QPushButton { min-width: 36dip; max-width: 36dip; min-height: 36dip; max-height: 36dip;"
"border-image: url(:/voip/volume_100); }"
"QPushButton:hover { border-image: url(:/voip/volume_100_hover); }";

const QString buttonSoundDisable =
"QPushButton { min-width: 36dip; max-width: 36dip; min-height: 36dip; max-height: 36dip;"
"border-image: url(:/voip/volume_off_100); }"
"QPushButton:hover { border-image: url(:/voip/volume_off_100_hover); }";

const QString loginNameLable = "*{ color: #000000; }";

const QString buttonAddChat =
"QPushButton { min-width: 36dip; max-width: 36dip; min-height: 36dip; max-height: 36dip;"
"border-image: url(:/voip/add_100); }"
"QPushButton:hover { border-image: url(:/voip/add_100_hover); }";

const QString buttonScreenSharingEnable =
"QPushButton { min-width: 36dip; max-width: 36dip; min-height: 36dip; max-height: 36dip;"
"border-image: url(:/voip/screen_100); }"
"QPushButton:hover { border-image: url(:/voip/screen_100_hover); }";

const QString buttonScreenSharingDisable =
"QPushButton { min-width: 36dip; max-width: 36dip; min-height: 36dip; max-height: 36dip;"
"border-image: url(:/voip/screen_off_100); }"
"QPushButton:hover { border-image: url(:/voip/screen_off_100_hover); }";


namespace
{
    enum
    {
        kmenu_item_volume = 0,
        kmenu_item_mic = 1,
        kmenu_item_cam = 2
    };
}

#define TOP_PANEL_COLOR                  QColor("#fddc6f")
#define BOTTOM_PANEL_BETWEEN_BUTTONS_GAP Utils::scale_value(24)
#define CALLING_TEXT                     "Calling..."
#define CONNECTING_TEXT                  "Connecting..."

extern std::string getFotmatedTime(unsigned ts);

Ui::SliderEx::SliderEx(QWidget* _parent)
    : QWidget(_parent)
{
    horizontalLayout_ = Utils::emptyHLayout(this);

    sliderIcon_ = new voipTools::BoundBox<PushButton_t>(this);
    sliderIcon_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    horizontalLayout_->addWidget(sliderIcon_);

    slider_ = new voipTools::BoundBox<QSlider>(this);
    slider_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    slider_->setOrientation(Qt::Horizontal);

    horizontalLayout_->addWidget(slider_);
    sliderIcon_->setText(QString(), QString());

    QMetaObject::connectSlotsByName(this);

    slider_->setMaximum(100);
    slider_->setMinimum(0);

    connect(slider_, SIGNAL(valueChanged(int)), this, SLOT(onVolumeChanged(int)), Qt::QueuedConnection);
    connect(slider_, SIGNAL(sliderReleased()), this, SLOT(onVolumeReleased()), Qt::QueuedConnection);
    connect(sliderIcon_, SIGNAL(clicked()), this, SLOT(onIconClicked()), Qt::QueuedConnection);
}

Ui::SliderEx::~SliderEx()
{

}

void Ui::SliderEx::setIconSize(const int _w, const int _h)
{
    if (!!sliderIcon_)
    {
        sliderIcon_->setIconSize(_w, _h);
    }
}

void Ui::SliderEx::onVolumeChanged(int _v)
{
    emit onSliderValueChanged(_v);
}

void Ui::SliderEx::onVolumeReleased()
{
    emit onSliderReleased();
}

void Ui::SliderEx::onIconClicked()
{
    emit onIconClick();
}

void Ui::SliderEx::setEnabled(bool _en)
{
    slider_->setEnabled(_en);
}

void Ui::SliderEx::setValue(int _v)
{
    slider_->setValue(_v);
}

void Ui::SliderEx::setPropertyForIcon(const char* _name, bool _val)
{
    sliderIcon_->setProperty(_name, _val);
    sliderIcon_->setStyle(QApplication::style());
}

void Ui::SliderEx::setIconForState(const PushButton_t::eButtonState _state, const std::string& _image)
{
    if (!!sliderIcon_)
    {
        sliderIcon_->setImageForState(_state, _image);
    }
}

void Ui::SliderEx::setPropertyForSlider(const char* _name, bool _val)
{
    slider_->setProperty(_name, _val);
    slider_->setStyle(QApplication::style());
}

Ui::CallPanelMainEx::CallPanelMainEx(QWidget* _parent, const CallPanelMainFormat& _panelFormat)
    : QWidget(_parent)
    , format_(_panelFormat)
    , rootWidget_(new QWidget(this))
    , nameLabel_(nullptr)
    , vVolControl_(this, false, true, vertSoundBg, [] (QPushButton& _btn, bool _muted)
    {
        if (_muted)
        {
            Utils::ApplyStyle(&_btn, buttonSoundDisable);
        }
        else
        {
            Utils::ApplyStyle(&_btn, buttonSoundEnable);
        }
    })
    , hVolControl_(this, true, true, horSoundBg, [] (QPushButton& _btn, bool _muted)
    {
        if (_muted)
        {
            Utils::ApplyStyle(&_btn, buttonSoundDisable);
        }
        else
        {
            Utils::ApplyStyle(&_btn, buttonSoundEnable);
        }
    })
    , isScreenSharingEnabled_(false)
    , localVideoEnabled_(false)
    , isCameraEnabled_(true)
    , buttonMaximize_(nullptr)
    , buttonLocalCamera_(nullptr)
    , buttonLocalMic_(nullptr)
    , screenSharing_(nullptr)
{
    setFixedHeight(format_.bottomPartHeight);
    {
        QVBoxLayout* l = Utils::emptyVLayout();
        setLayout(l);
    }

    { // we need root widget to make transcluent window
        layout()->addWidget(rootWidget_);
    }

    {
        QVBoxLayout* l = Utils::emptyVLayout();
        rootWidget_->setLayout(l);
    }

    QString style = QString("QWidget { background-color: %1; }").arg(TOP_PANEL_COLOR.name());
    Utils::ApplyStyle(rootWidget_, style);

    setCursor(QCursor(Qt::PointingHandCursor));

    { // bottom part
        QWidget* bottomPartWidget = new voipTools::BoundBox<QWidget>(rootWidget_);
        bottomPartWidget->setFixedHeight(format_.bottomPartHeight);
        QHBoxLayout* layout = Utils::emptyHLayout();
        bottomPartWidget->setLayout(layout);
        layout->setAlignment(Qt::AlignCenter);

        if (format_.bottomPartFormat & kVPH_ShowName)
        {
            nameLabel_ = new TextEmojiWidget(bottomPartWidget, Fonts::appFontScaled(14, Fonts::FontWeight::Medium), QColor(0, 0, 0));
            nameLabel_->disableFixedPreferred();
            nameLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
            nameLabel_->setEllipsis(true);

            layout->addSpacing(Utils::scale_value(16));
            layout->addWidget(nameLabel_, 1, Qt::AlignLeft);
        }

        buttonAddUser_ = addButton<QPushButton>(*bottomPartWidget, buttonAddChat, SLOT(addUserToVideoCall()));
        buttonAddUser_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Add users to call"));
        layout->addSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);

        buttonGoChat_ = addButton<QPushButton>(*bottomPartWidget, buttonGoChat,        SLOT(onClickGoChat()));
        buttonGoChat_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Open chat page"));
        layout->addSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);

#ifndef __linux__
        screenSharing_ = addButton<QPushButton>(*bottomPartWidget, buttonScreenSharingDisable, SLOT(onScreenSharing()));
        layout->addSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);
#endif

        buttonStopCall_ = addButton<QPushButton>(*bottomPartWidget, buttonStopCall,      SLOT(onStopCall()));
        buttonStopCall_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Stop call"));
        layout->addSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);

        buttonLocalCamera_ = addButton<QPushButton>(*bottomPartWidget, buttonCameraDisable, SLOT(onCameraTurn()));
        layout->addSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);

        buttonLocalMic_ = addButton<QPushButton>(*bottomPartWidget, buttonMicDisable,    SLOT(onMicTurn()));
        layout->addSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);

        // Volume controls group.
        volumeGroup = addVolumeGroup(*bottomPartWidget, false, Utils::scale_value(660));

        if (format_.bottomPartFormat & kVPH_ShowTime)
        {
            timeLabel_ = new TextEmojiWidget(bottomPartWidget, Fonts::appFontScaled(12, Fonts::FontWeight::Medium), QColor(0, 0, 0));
            timeLabel_->disableFixedPreferred();
            timeLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
            timeLabel_->setEllipsis(true);

            timeLabel_->setText(QT_TRANSLATE_NOOP("voip_pages", CALLING_TEXT));

            // Make disable text color also black.
            Utils::ApplyStyle(timeLabel_, loginNameLable);

            layout->addWidget(timeLabel_, 1, Qt::AlignRight);
            layout->addSpacing(Utils::scale_value(16));
        }

        rootWidget_->layout()->addWidget(bottomPartWidget);
    }

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const voip_manager::ContactsList&)), this, SLOT(onVoipCallNameChanged(const voip_manager::ContactsList&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), this, SLOT(onVoipMediaLocalVideo(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalAudio(bool)), this, SLOT(onVoipMediaLocalAudio(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallTimeChanged(unsigned,bool)), this, SLOT(onVoipCallTimeChanged(unsigned,bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipVideoDeviceSelected(const voip_proxy::device_desc&)), this, SLOT(onVoipVideoDeviceSelected(const voip_proxy::device_desc&)), Qt::DirectConnection);

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallOutAccepted(const voip_manager::ContactEx&)), this, SLOT(onVoipCallOutAccepted(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallIncomingAccepted(const voip_manager::ContactEx&)), this, SLOT(onVoipCallIncomingAccepted(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallConnected(const voip_manager::ContactEx&)), this, SLOT(onVoipCallConnected(const voip_manager::ContactEx&)), Qt::DirectConnection);

    hVolControl_.hide();
    vVolControl_.hide();

    updateVideoDeviceButtonsState();
}

Ui::CallPanelMainEx::~CallPanelMainEx()
{

}

void Ui::CallPanelMainEx::onSoundTurn()
{
    assert(false);
}

void Ui::CallPanelMainEx::onMicTurn()
{
    Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();
}

void Ui::CallPanelMainEx::onStopCall()
{
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::CallPanelMainEx::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
{
    if(_contacts.contacts.empty())
    {
        return;
    }

    activeContact_ = _contacts.contacts[0].contact;
    QString friendlyName = Logic::getContactListModel()->getDisplayName(activeContact_.c_str());

    // Hide if max users.
    buttonAddUser_->setVisible(_contacts.contacts.size() < Ui::GetDispatcher()->getVoipController().maxVideoConferenceMembers() - 1);

    if (nameLabel_)
    {
        nameLabel_->setText(friendlyName);
    }
}

void Ui::CallPanelMainEx::onClickGoChat()
{
    emit onClickOpenChat(activeContact_);
}

void Ui::CallPanelMainEx::onVoipMediaLocalVideo(bool _enabled)
{
    localVideoEnabled_ = _enabled;

    updateVideoDeviceButtonsState();
}

void Ui::CallPanelMainEx::onVoipMediaLocalAudio(bool _enabled)
{
    if (buttonLocalMic_)
    {
        if (_enabled)
        {
            Utils::ApplyStyle(buttonLocalMic_, buttonMicEnable);
            buttonLocalMic_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Turn off microphone"));
        }
        else
        {
            Utils::ApplyStyle(buttonLocalMic_, buttonMicDisable);
            buttonLocalMic_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Turn on microphone"));
        }
    }
}

void Ui::CallPanelMainEx::onCameraTurn()
{
    Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}

template<typename ButtonType>
ButtonType* Ui::CallPanelMainEx::addButton(
    QWidget& _parentWidget,
    const QString& _propertyName,
    const char* _slot,
    bool _bDefaultCursor,
    bool rightAlignment)
{
    ButtonType* btn = new voipTools::BoundBox<ButtonType>(&_parentWidget);
    Utils::ApplyStyle(btn, _propertyName);
    btn->setSizePolicy(rightAlignment ? QSizePolicy::Expanding : QSizePolicy::Preferred, QSizePolicy::Expanding);

    // For volume control we have blinking with cursor under mac.
    // We will use dfault cursor to fix it.
    if (!_bDefaultCursor)
    {
        btn->setCursor(QCursor(Qt::PointingHandCursor));
    }
    btn->setFlat(true);

    if (!rightAlignment)
    {
        _parentWidget.layout()->addWidget(btn);
    }
    else
    {
        QBoxLayout* layout = qobject_cast<QBoxLayout*>(_parentWidget.layout());
        if (layout)
        {
            qobject_cast<QBoxLayout*>(_parentWidget.layout())->addWidget(btn, 1, Qt::AlignRight);
        }
    }
    connect(btn, SIGNAL(clicked()), this, _slot, Qt::QueuedConnection);

    // Disable dragging using using buttons.
    btn->setAttribute(Qt::WA_NoMousePropagation);

    return btn;
}

Ui::VolumeGroup* Ui::CallPanelMainEx::addVolumeGroup(QWidget& _parentWidget,  bool rightAlignment, int verticalSize)
{
    VolumeGroup* volumeGroup = new VolumeGroup(&_parentWidget, true, [] (QPushButton& _btn, bool _muted)
    {
        if (_muted)
        {
            Utils::ApplyStyle(&_btn, buttonSoundDisable);
        }
        else
        {
            Utils::ApplyStyle(&_btn, buttonSoundEnable);
        }
    }, verticalSize);

    volumeGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    if (!rightAlignment)
    {
        QBoxLayout* layout = qobject_cast<QBoxLayout*>(_parentWidget.layout());
        if (layout)
        {
            layout->addWidget(volumeGroup, 0, Qt::AlignVCenter);
        }
    }
    else
    {
        QBoxLayout* layout = qobject_cast<QBoxLayout*>(_parentWidget.layout());
        if (layout)
        {
            qobject_cast<QBoxLayout*>(_parentWidget.layout())->addWidget(volumeGroup, 1, Qt::AlignRight);
        }
    }

    // Disable dragging using using buttons.
    volumeGroup->setAttribute(Qt::WA_NoMousePropagation);

    return volumeGroup;
}


void Ui::CallPanelMainEx::hideEvent(QHideEvent* _e)
{
    QWidget::hideEvent(_e);
    hVolControl_.hide();
    vVolControl_.hide();
}

void Ui::CallPanelMainEx::moveEvent(QMoveEvent* _e)
{
    QWidget::moveEvent(_e);
    hVolControl_.hide();
    vVolControl_.hide();
}

void Ui::CallPanelMainEx::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
//    hVolControl_.hide();
//    vVolControl_.hide();
}

void Ui::CallPanelMainEx::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);
    hVolControl_.hide();
    vVolControl_.hide();
}

void Ui::CallPanelMainEx::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
{
    if (_contactEx.call_count <= 1)
    {
        voip_manager::CipherState state;
        state.state = voip_manager::CipherState::kCipherStateFailed;
        timeLabel_->setText(QT_TRANSLATE_NOOP("voip_pages", CALLING_TEXT));
    }
}

void Ui::CallPanelMainEx::onVoipCallOutAccepted(const voip_manager::ContactEx& _contactEx)
{
    if (timeLabel_->text() == QT_TRANSLATE_NOOP("voip_pages", CALLING_TEXT))
    {
        timeLabel_->setText(QT_TRANSLATE_NOOP("voip_pages", CONNECTING_TEXT));
    }
}

void Ui::CallPanelMainEx::onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contactEx)
{
    if (timeLabel_->text() == QT_TRANSLATE_NOOP("voip_pages", CALLING_TEXT))
    {
        timeLabel_->setText(QT_TRANSLATE_NOOP("voip_pages", CONNECTING_TEXT));
    }
}

void Ui::CallPanelMainEx::onVoipCallConnected(const voip_manager::ContactEx& _contactEx)
{
    if (timeLabel_->text() == QT_TRANSLATE_NOOP("voip_pages", CONNECTING_TEXT))
    {
        timeLabel_->setText(getFotmatedTime(0).c_str());
    }
}


void Ui::CallPanelMainEx::onVoipCallTimeChanged(unsigned _secElapsed, bool /*have_call*/)
{
    if (timeLabel_ && _secElapsed > 0)
    {
        timeLabel_->setText(getFotmatedTime(_secElapsed).c_str());
    }
}

void Ui::CallPanelMainEx::addUserToVideoCall()
{
    showAddUserToVideoConverenceDialogMainWindow(this, parentWidget());
}

void Ui::CallPanelMainEx::onScreenSharing()
{
    const QList<voip_proxy::device_desc>& screens = Ui::GetDispatcher()->getVoipController().screenList();
    int screenIndex = 0;

    if (!isScreenSharingEnabled_ && screens.size() > 1)
    {
        ContextMenu menu(this);
        ContextMenu::applyStyle(&menu, false, Utils::scale_value(14), Utils::scale_value(28));

        for (int i = 0; i < screens.size(); i++)
        {
            menu.addAction(QT_TRANSLATE_NOOP("voip_pages", "Screen") % ql1c(' ') % QString::number(i + 1), [i, this, screens]() {
                isScreenSharingEnabled_ = !isScreenSharingEnabled_;
                Ui::GetDispatcher()->getVoipController().switchShareScreen(!screens.empty() ? &screens[i] : nullptr);
                updateVideoDeviceButtonsState();
            });
        }

        menu.exec(QCursor::pos());
    }
    else
    {
        isScreenSharingEnabled_ = !isScreenSharingEnabled_;
        Ui::GetDispatcher()->getVoipController().switchShareScreen(!screens.empty() ? &screens[screenIndex] : nullptr);
        updateVideoDeviceButtonsState();
    }
}

void Ui::CallPanelMainEx::mouseReleaseEvent(QMouseEvent *event)
{
    emit onBackToVideo();
}

void Ui::CallPanelMainEx::onVoipVideoDeviceSelected(const voip_proxy::device_desc& desc)
{
    isScreenSharingEnabled_ = (desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && desc.video_dev_type == voip_proxy::kvoipDeviceDesktop);
    isCameraEnabled_        = (desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && desc.video_dev_type == voip_proxy::kvoipDeviceCamera);

    updateVideoDeviceButtonsState();
}

void Ui::CallPanelMainEx::updateVideoDeviceButtonsState()
{
    bool enableCameraButton = localVideoEnabled_ && isCameraEnabled_;
    bool enableScreenButton = localVideoEnabled_ && isScreenSharingEnabled_;

    if (buttonLocalCamera_)
    {
        if (enableCameraButton)
        {
            Utils::ApplyStyle(buttonLocalCamera_, buttonCameraEnable);
            buttonLocalCamera_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Turn off camera"));
        }
        else
        {
            Utils::ApplyStyle(buttonLocalCamera_, buttonCameraDisable);
            buttonLocalCamera_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Turn on camera"));
        }
    }

    if (screenSharing_)
    {
        if (enableScreenButton)
        {
            Utils::ApplyStyle(screenSharing_, buttonScreenSharingEnable);
            buttonLocalCamera_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Turn off screen sharing"));
        }
        else
        {
            Utils::ApplyStyle(screenSharing_, buttonScreenSharingDisable);
            buttonLocalCamera_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Turn on screen sharing"));
        }
    }
}
