#include "stdafx.h"
#include "MainMenu.h"
#include "MainWindow.h"
#include "MainPage.h"
#include "sidebar/SidebarUtils.h"
#include "mplayer/FFMpegPlayer.h"
#include "../controls/CommonStyle.h"
#include "../controls/CustomButton.h"
#include "../controls/ContactAvatarWidget.h"
#include "../controls/TextEditEx.h"
#include "../controls/LabelEx.h"
#include "../controls/TransparentScrollBar.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../utils/Text2DocConverter.h"
#include "../fonts.h"
#include "../my_info.h"
#include "../gui_settings.h"
#include "../core_dispatcher.h"

namespace
{
    const int ITEM_HEIGHT = 40;
    const int CLOSE_REGION_SIZE = 48;
    const int HOR_PADDING = 16;
    const int LINE_LEFT_MARGIN = 52;
    const int VER_PADDING = 16;
    const int TEXT_OFFSET = 12;
    const int BACKGROUND_HEIGHT_MAX = 148;
    const int BACKGROUND_HEIGHT_MIN = 116;
    const int CLOSE_TOP_PADDING = 16;
    const int CLOSE_WIDTH = 16;
    const int CLOSE_HEIGHT = 16;
    const int AVATAR_SIZE = 52;
    const int AVATAR_BOTTOM_PADDING = 16;
    const int NAME_PADDING = 12;
    const int CHECKBOX_WIDTH = 44;
    const int CHECKBOX_HEIGHT = 20;
    const int NAME_TOP_PADDING = 6;
    const int STATUS_TOP_PADDING = 8;
}

namespace Ui
{
    BackWidget::BackWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        setMinimumHeight(Utils::scale_value(BACKGROUND_HEIGHT_MIN));
        setMaximumHeight(Utils::scale_value(BACKGROUND_HEIGHT_MAX));
    }

    void BackWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        emit clicked();
        QWidget::mouseReleaseEvent(e);
    }

    void BackWidget::paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        p.fillRect(rect(), CommonStyle::getColor(CommonStyle::Color::GREEN_FILL));
    }

    void BackWidget::resizeEvent(QResizeEvent *e)
    {
        emit resized();
        QWidget::resizeEvent(e);
    }

    MainMenu::MainMenu(QWidget* _parent)
        : QWidget(_parent)
        , Parent_(_parent)
    {
        setObjectName(qsl("menu"));
        setStyleSheet(Utils::LoadStyle(qsl(":/qss/main_menu")));

        ScrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
        ScrollArea_->setContentsMargins(0, 0, 0, 0);
        ScrollArea_->setWidgetResizable(true);

        MainWidget_ = new QWidget(ScrollArea_);
        ScrollArea_->setWidget(MainWidget_);

        MainWidget_->setStyleSheet(qsl("background: transparent; border: none;"));
        ScrollArea_->setStyleSheet(qsl("background-color: %1; border: none;").arg(CommonStyle::getFrameColor().name()));

        auto mainLayout = Utils::emptyVLayout(MainWidget_);
        mainLayout->setAlignment(Qt::AlignLeft);

        Background_ = new BackWidget(MainWidget_);
        mainLayout->addWidget(Background_, 1);
        connect(Background_, &BackWidget::resized, this, &MainMenu::resize, Qt::QueuedConnection);

        Close_ = new CustomButton(MainWidget_, qsl(":/controls/close_big_b_100"));
        Close_->setFixedSize(Utils::scale_value(CLOSE_WIDTH), Utils::scale_value(CLOSE_HEIGHT));
        Close_->setAttribute(Qt::WA_TransparentForMouseEvents);

        Avatar_ = new ContactAvatarWidget(MainWidget_, MyInfo()->aimId(), MyInfo()->friendlyName(), Utils::scale_value(AVATAR_SIZE), true);
        Avatar_->setAttribute(Qt::WA_TransparentForMouseEvents);
        Avatar_->SetOutline(true);

        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(Utils::scale_value(2));
        shadow->setXOffset(0);
        shadow->setYOffset(Utils::scale_value(1));
        shadow->setColor(QColor(0, 0, 0, 77));
        Avatar_->setGraphicsEffect(shadow);

        Name_ = new TextEditEx(MainWidget_, Fonts::appFontScaled(17, Fonts::FontWeight::Medium), QColor(ql1s("#ffffff")), false, false);
        Name_->setFrameStyle(QFrame::NoFrame);
        Name_->setStyleSheet(qsl("background: transparent;"));
        Name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Name_->setAttribute(Qt::WA_TransparentForMouseEvents);

        Status_ = new TextEditEx(MainWidget_, Fonts::appFontScaled(14, Fonts::FontWeight::Normal), QColor(ql1s("#ffffff")), false, false);
        Status_->setFrameStyle(QFrame::NoFrame);
        Status_->setStyleSheet(qsl("background: transparent;"));
        Status_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Status_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Status_->setAttribute(Qt::WA_TransparentForMouseEvents);

        auto buttonsLayout = Utils::emptyVLayout(MainWidget_);
        buttonsLayout->setAlignment(Qt::AlignLeft);

        CreateGroupchat_ = new ActionButton(MainWidget_, qsl(":/resources/create_chat_100.png"), QT_TRANSLATE_NOOP("burger_menu", "Create group chat"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        CreateGroupchat_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(CreateGroupchat_);

        AddContact_ = new ActionButton(MainWidget_, qsl(":/resources/add_contact_100.png"), QT_TRANSLATE_NOOP("burger_menu", "Add contact"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        AddContact_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(AddContact_);

        Contacts_ = new ActionButton(MainWidget_, qsl(":/resources/contacts_100.png"), QT_TRANSLATE_NOOP("burger_menu", "Contacts"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        Contacts_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(Contacts_);

        auto horLayout = Utils::emptyHLayout();
        SoundsButton_ = new CustomButton(MainWidget_, qsl(":/resources/settings/settings_notify_100.png"));
        SoundsButton_->setOffsets(Utils::scale_value(TEXT_OFFSET), 0);
        SoundsButton_->setText(QT_TRANSLATE_NOOP("burger_menu", "Sounds"));
        SoundsButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        SoundsButton_->setAlign(Qt::AlignLeft);
        SoundsButton_->setFocusPolicy(Qt::NoFocus);
        SoundsButton_->setFixedHeight(Utils::scale_value(ITEM_HEIGHT));
        SoundsButton_->adjustSize();
        SoundsButton_->setTextColor(qsl("#454545"));
        horLayout->addWidget(SoundsButton_);
        SoundsCheckbox_ = new QCheckBox(MainWidget_);
        SoundsCheckbox_->setObjectName(qsl("greenSwitcher"));
        SoundsCheckbox_->adjustSize();
        SoundsCheckbox_->setCursor(QCursor(Qt::PointingHandCursor));
        SoundsCheckbox_->setFixedSize(Utils::scale_value(CHECKBOX_WIDTH), Utils::scale_value(CHECKBOX_HEIGHT));
        SoundsCheckbox_->setChecked(get_gui_settings()->get_value<bool>(settings_sounds_enabled, true));
        SoundsCheckbox_->setStyleSheet(qsl("outline: none;"));
        horLayout->addWidget(SoundsCheckbox_);
        horLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
        buttonsLayout->addLayout(horLayout);

        Line_ = new LineWidget(MainWidget_, Utils::scale_value(LINE_LEFT_MARGIN), 0, 0, 0);
        buttonsLayout->addWidget(Line_);

        Settings_ = new ActionButton(MainWidget_, qsl(":/resources/settings/settings_general_100.png"), QT_TRANSLATE_NOOP("burger_menu", "Settings"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        Settings_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(Settings_);

        SignOut_ = new ActionButton(MainWidget_, qsl(":/resources/settings/settings_signout_100.png"), QT_TRANSLATE_NOOP("burger_menu", "Sign out"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        SignOut_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(SignOut_);
        buttonsLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
        mainLayout->addLayout(buttonsLayout);

        auto hLayout = Utils::emptyHLayout();
        hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(HOR_PADDING), 0, QSizePolicy::Fixed));

        auto f = Fonts::appFontScaled(13);

        About_ = new LabelEx(MainWidget_);
        About_->setText(QT_TRANSLATE_NOOP("burger_menu", "About app"));
        About_->setCursor(Qt::PointingHandCursor);
        About_->setFont(f);
        About_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        hLayout->addWidget(About_);

        hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(HOR_PADDING), 0, QSizePolicy::Fixed));

        ContactUs_ = new LabelEx(MainWidget_);
        ContactUs_->setText(QT_TRANSLATE_NOOP("burger_menu", "Contact Us"));
        ContactUs_->setCursor(Qt::PointingHandCursor);
        ContactUs_->setFont(f);
        ContactUs_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        hLayout->addWidget(ContactUs_);
        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        mainLayout->addLayout(hLayout);
        mainLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(VER_PADDING), QSizePolicy::Preferred, QSizePolicy::Fixed));

        auto l = Utils::emptyVLayout(this);
        l->addWidget(ScrollArea_);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, &MainMenu::Hide, Qt::QueuedConnection);
        connect(MyInfo(), &Ui::my_info::received, this, &MainMenu::myInfoUpdated, Qt::QueuedConnection);
        connect(SoundsCheckbox_, &QCheckBox::stateChanged, this, &MainMenu::soundsChecked, Qt::QueuedConnection);
        connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, &MainMenu::guiSettingsChanged, Qt::QueuedConnection);

        connect(CreateGroupchat_, &Ui::ActionButton::clicked, this, &MainMenu::createGroupChat, Qt::QueuedConnection);
        connect(AddContact_, &Ui::ActionButton::clicked, this, &MainMenu::addContact, Qt::QueuedConnection);
        connect(Contacts_, &Ui::ActionButton::clicked, this, &MainMenu::contacts, Qt::QueuedConnection);
        connect(Settings_, &Ui::ActionButton::clicked, this, &MainMenu::settings, Qt::QueuedConnection);
        connect(About_, &Ui::LabelEx::clicked, this, &MainMenu::about, Qt::QueuedConnection);
        connect(ContactUs_, &Ui::LabelEx::clicked, this, &MainMenu::contactUs, Qt::QueuedConnection);
        connect(SignOut_, &Ui::ActionButton::clicked, this, &MainMenu::signOut, Qt::QueuedConnection);

        connect(AddContact_, &Ui::ActionButton::clicked, this, &MainMenu::Hide, Qt::QueuedConnection);
        connect(Settings_, &Ui::ActionButton::clicked, this, &MainMenu::Hide, Qt::QueuedConnection);
        connect(About_, &Ui::LabelEx::clicked, this, &MainMenu::Hide, Qt::QueuedConnection);
        connect(ContactUs_, &Ui::LabelEx::clicked, this, &MainMenu::Hide, Qt::QueuedConnection);
    }

    void MainMenu::updateState()
    {
        QFontMetrics f(Status_->font());
        auto elidedState = f.elidedText(MyInfo()->state(), Qt::ElideRight, width() - Utils::scale_value(HOR_PADDING + AVATAR_SIZE + HOR_PADDING + NAME_PADDING));

        auto &doc = *Status_->document();
        doc.clear();

        QTextCursor cursor = Status_->textCursor();
        Logic::Text2Doc(elidedState, cursor, Logic::Text2DocHtmlMode::Pass, false);
        Logic::FormatDocument(doc, f.height());
        Status_->adjustHeight(f.width(elidedState) + Utils::scale_value(HOR_PADDING));
    }

    void MainMenu::Hide()
    {
        auto w = Utils::InterConnector::instance().getMainWindow();
        if (w)
            w->hideMenu();
    }

    void MainMenu::myInfoUpdated()
    {
        Avatar_->UpdateParams(MyInfo()->aimId(), MyInfo()->friendlyName());
        QFontMetrics f(Name_->font());
        auto elidedName = f.elidedText(MyInfo()->friendlyName(), Qt::ElideRight, width() - Utils::scale_value(HOR_PADDING + AVATAR_SIZE + HOR_PADDING + NAME_PADDING));

        auto &doc = *Name_->document();
        doc.clear();

        QTextCursor cursor = Name_->textCursor();
        Logic::Text2Doc(elidedName, cursor, Logic::Text2DocHtmlMode::Pass, false);
        Logic::FormatDocument(doc, f.height());
        Name_->adjustHeight(f.width(elidedName) + Utils::scale_value(HOR_PADDING));

        updateState();
    }

    void MainMenu::resizeEvent(QResizeEvent *e)
    {
        resize();

        return QWidget::resizeEvent(e);
    }

    void MainMenu::resize()
    {
        Background_->setFixedWidth(width());
        CreateGroupchat_->setFixedWidth(width());
        AddContact_->setFixedWidth(width());
        Contacts_->setFixedWidth(width());
        Settings_->setFixedWidth(width());

        SoundsButton_->setFixedWidth(width() - Utils::scale_value(HOR_PADDING + CHECKBOX_WIDTH));
        Close_->move(Utils::scale_value(HOR_PADDING), Utils::scale_value(CLOSE_TOP_PADDING));
        Avatar_->move(Utils::scale_value(HOR_PADDING), Background_->height() - Utils::scale_value(AVATAR_BOTTOM_PADDING + AVATAR_SIZE));
        Name_->setFixedWidth(width() - Utils::scale_value(HOR_PADDING + AVATAR_SIZE + HOR_PADDING + NAME_PADDING));
        Name_->move(Utils::scale_value(HOR_PADDING + AVATAR_SIZE + NAME_PADDING), Background_->height() - Utils::scale_value(AVATAR_BOTTOM_PADDING + AVATAR_SIZE - NAME_TOP_PADDING));
        Line_->setLineWidth(width() - Utils::scale_value(HOR_PADDING + LINE_LEFT_MARGIN));

        Status_->setFixedWidth(width() - Utils::scale_value(HOR_PADDING + AVATAR_SIZE + HOR_PADDING + NAME_PADDING));
        Status_->move(Utils::scale_value(HOR_PADDING + AVATAR_SIZE + NAME_PADDING), Background_->height() - Utils::scale_value(AVATAR_BOTTOM_PADDING + AVATAR_SIZE - STATUS_TOP_PADDING) + Name_->height());
    }

    void MainMenu::showEvent(QShowEvent *e)
    {
        updateState();
        return QWidget::showEvent(e);
    }

    void MainMenu::hideEvent(QHideEvent *e)
    {
        return QWidget::hideEvent(e);
    }

    void MainMenu::mouseReleaseEvent(QMouseEvent *e)
    {
        static auto closeRect = QRect(0, 0, Utils::scale_value(CLOSE_REGION_SIZE), Utils::scale_value(CLOSE_REGION_SIZE));
        if (closeRect.contains(e->pos()))
        {
            Hide();
            return;
        }

        auto profileRect = QRect(0, Avatar_->y(), width(), Avatar_->height());
        if (profileRect.contains(e->pos()))
        {
            emit myProfile();
            Hide();
            return;
        }

        return QWidget::mouseReleaseEvent(e);
    }

    void MainMenu::soundsChecked(int value)
    {
        get_gui_settings()->set_value<bool>(settings_sounds_enabled, value != Qt::Unchecked);
    }

    void MainMenu::guiSettingsChanged(QString value)
    {
        if (value != settings_sounds_enabled)
            return;

        QSignalBlocker sb(SoundsCheckbox_);
        SoundsCheckbox_->setChecked(get_gui_settings()->get_value<bool>(settings_sounds_enabled, true));
    }

    void MainMenu::signOut()
    {
        QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to sign out?");
        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Sign out"),
            nullptr);

        if (confirm)
        {
            get_gui_settings()->set_value(settings_feedback_email, QString());
            GetDispatcher()->post_message_to_core(qsl("logout"), nullptr);
            emit Utils::InterConnector::instance().logout();
        }
    }

    MainMenu::~MainMenu()
    {
    }
}
