#include "stdafx.h"
#include "ProfilePage.h"
#include "SidebarUtils.h"
#include "../GroupChatOperations.h"
#include "../MainPage.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/contact_profile.h"
#include "../contact_list/SearchMembersModel.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../livechats/LiveChatMembersControl.h"
#include "../../my_info.h"
#include "../../core_dispatcher.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../controls/CustomButton.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/FlatMenu.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/ContextMenu.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/utils.h"
#include "../../cache/countries.h"

namespace
{
    const int avatar_size = 180;
    const int HOR_PADDING = 16;
    const int LEFT_MARGIN = 52;
    const int button_size = 40;
    const int big_button_size = 48;
    const int back_button_spacing = 8;
    const int members_count = 5;
    const int buttons_spacing = 32;
    const int buttons_top_margin = 24;
    const int buttons_margin = 12;
    const int button_height = 40;
    const int button_offset = 28;
    const int avatar_botton_spacing = 10;
    const int name_margin = 18;
    const int edit_top_margin = 14;
    const int save_button_offset = 16;
    const int name_desc_space = 40;
}

namespace Ui
{
    InfoPlate::InfoPlate(QWidget* parent, int leftMargin)
        : QWidget(parent)
        , attachPhone_(false)
    {
        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->setContentsMargins(leftMargin, 0, 0, 0);
        {
            auto vLayout = Utils::emptyVLayout();
            vLayout->setContentsMargins(0, 0, 0, Utils::scale_value(HOR_PADDING));
            header_ = new LabelEx(this);
            header_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
            header_->setFont(Fonts::appFontScaled(14));
            vLayout->addWidget(header_);
            info_ = new TextEmojiWidget(this, Fonts::appFontScaled(18), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
            connect(info_, &TextEmojiWidget::rightClicked, this, &InfoPlate::menuRequested, Qt::QueuedConnection);
            info_->setSelectable(true);
            vLayout->addWidget(info_);
            phoneInfo_ = new LabelEx(this);
            phoneInfo_->setWordWrap(true);
            phoneInfo_->setFont(Fonts::appFontScaled(14));
            phoneInfo_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
            vLayout->addWidget(phoneInfo_);
            phoneInfo_->hide();
            rootLayout->addLayout(vLayout);
            Utils::grabTouchWidget(header_);
            Utils::grabTouchWidget(info_);
            Utils::grabTouchWidget(phoneInfo_);
            Utils::grabTouchWidget(this);
        }
    }

    void InfoPlate::menuRequested()
    {
        ContextMenu* menu = new ContextMenu(this);
        menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/copy_100"))), QT_TRANSLATE_NOOP("context_menu", "Copy"), QVariant());
        connect(menu, &ContextMenu::triggered, this, &InfoPlate::menu, Qt::QueuedConnection);
        connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        menu->popup(QCursor::pos());
    }

    void InfoPlate::menu(QAction*)
    {
        QApplication::clipboard()->setText(infoStr_);
    }

    void InfoPlate::setAttachPhone(bool value)
    {
        attachPhone_ = value;
        info_->setSelectable(!value);
        if (value)
        {
            info_->setText(QT_TRANSLATE_NOOP("sidebar", "Attach phone"), CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
            info_->adjustSize();
            connect(info_, &TextEmojiWidget::clicked, this, []()
            {
                emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_AttachPhone);
            });
            phoneInfo_->setText(QT_TRANSLATE_NOOP("sidebar", "for safety and spam protection"));
            phoneInfo_->show();
        }
        else
        {
            setInfo(infoStr_);
            disconnect(info_, &TextEmojiWidget::clicked, this, nullptr);
            phoneInfo_->setText(QT_TRANSLATE_NOOP("sidebar", "Only visible for those, who has it in the phone book"));
            phoneInfo_->show();
        }
        info_->setCursor(value ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void InfoPlate::setHeader(const QString& header)
    {
        header_->setText(header);
        header_->adjustSize();
    }

    void InfoPlate::setInfo(const QString& info, const QString& prefix)
    {
        infoStr_ = info;
        if (!infoStr_.startsWith(prefix))
            infoStr_ = prefix + infoStr_;

        info_->setText(infoStr_, QColor(qsl("#454545")));
        info_->adjustSize();
        phoneInfo_->hide();
    }

    void InfoPlate::elideText(int width)
    {
        if (attachPhone_)
            return;

        QFontMetrics m(Fonts::appFontScaled(16));
        QString result = m.elidedText(infoStr_, Qt::ElideRight, width);
        info_->setText(result);
        info_->setSourceText(infoStr_);
    }

    QString InfoPlate::getInfoText() const
    {
        return info_ ? info_->text() : QString();
    }

    ProfilePage::ProfilePage(QWidget* parent)
        : SidebarPage(parent)
        , info_(std::make_shared<Data::ChatInfo>())
        , connectOldVisible_(false)
        , myInfo_(false)
    {
        init();
    }

    void ProfilePage::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        QWidget::paintEvent(_e);
        p.fillRect(rect(), CommonStyle::getFrameColor());
    }

    void ProfilePage::updateWidth()
    {
        const auto w = width_ - Utils::scale_value(LEFT_MARGIN + 2 * HOR_PADDING);
        Line_->setLineWidth(w);
        if (myInfo_ && width() >= Utils::scale_value(596))
        {
            name_->adjustHeight(w - editLabel_->width());
        }
        else if (width() >= Utils::scale_value(596))
        {
            nameEdit_->adjustHeight(w);
            descriptionEdit_->adjustHeight(w);
            saveButtonSpace_->setFixedWidth(width_ - Utils::scale_value(LEFT_MARGIN + HOR_PADDING + Utils::scale_value(8)) - saveButton_->width());
        }
        else
        {
            name_->adjustHeight(w);
            nameEdit_->adjustHeight(w);
            descriptionEdit_->adjustHeight(w);
            saveButtonSpace_->setFixedWidth(w - saveButton_->width());
        }

        ignoreWidget_->setFixedWidth(w);
        uin_->elideText(w);
        phone_->elideText(w);
        firstName_->elideText(w);
        lastName_->elideText(w);
        nickName_->elideText(w);
        birthday_->elideText(w);
        city_->elideText(w);
        country_->elideText(w);

        const auto btnWidth = width_ - Utils::scale_value(2 * HOR_PADDING);
        renameContact_->setFixedWidth(btnWidth);
        ignoreListButton->setFixedWidth(btnWidth);
        attachOldAcc->setFixedWidth(btnWidth);
        shareLink_->setFixedWidth(btnWidth);
        avatar_->update();
    }

    void ProfilePage::touchScrollStateChanged(QScroller::State st)
    {
        const bool isOn = st != QScroller::Inactive;
        name_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        nameEdit_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        descriptionEdit_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        uin_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        phone_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        firstName_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        lastName_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        nickName_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        birthday_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        city_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        country_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        renameContact_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        ignoreListButton->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        attachOldAcc->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        avatar_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        shareLink_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
    }

    void ProfilePage::copyLinkClicked()
    {
        if (currentAimId_.isEmpty())
            return;

        const QString link = ql1s("https://icq.com/") % currentAimId_;

        Ui::SelectContactsWidget shareDialog(
            nullptr,
            Logic::MembersWidgetRegim::SHARE_LINK,
            QT_TRANSLATE_NOOP("popup_window", "Share link"),
            QT_TRANSLATE_NOOP("popup_window", "COPY LINK"),
            Ui::MainPage::instance(),
            true);

        const auto action = shareDialog.show();
        if (action == QDialog::Accepted)
        {
            const auto contact = shareDialog.getSelectedContact();
            if (!contact.isEmpty())
            {
                Logic::getContactListModel()->setCurrent(contact, -1, true);
                Ui::GetDispatcher()->sendMessageToContact(contact, link);
            }
            else
                QApplication::clipboard()->setText(link);

            emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
        }
    }

    void ProfilePage::initFor(const QString& aimId)
    {
        const auto fixedAimId = aimId.isEmpty() ? MyInfo()->aimId() : aimId;
        bool newContact = (currentAimId_ != fixedAimId);
        currentAimId_ = fixedAimId;
        myInfo_ = (MyInfo()->aimId() == currentAimId_ && aimId.isEmpty());

        QString oldName = Logic::getContactListModel()->getDisplayName(fixedAimId);
        if (!newContact && myInfo_)
        {
            oldName = nickName_->getInfoText();
            if (!nickName_->getInfoText().isEmpty())
            {
                oldName = nickName_->getInfoText();
            }
        }

        Logic::GetAvatarStorage()->ForceRequest(currentAimId_, Utils::scale_value(avatar_size));

        avatar_->UpdateParams(currentAimId_, Logic::getContactListModel()->getDisplayName(currentAimId_));
        avatar_->update();

        core::coll_helper helper(GetDispatcher()->create_collection(), true);
        GetDispatcher()->post_message_to_core(qsl("load_flags"), helper.get());

        attachOldAcc->setVisible(myInfo_ && connectOldVisible_);
        backButton_->setVisible(!myInfo_);

        if (newContact)
        {
            chatEditWidget_->hide();
            phone_->hide();
            firstName_->hide();
            lastName_->hide();
            nickName_->hide();
            birthday_->hide();
            city_->hide();
            country_->hide();
        }


        name_->setPlainText(QString());
        QTextCursor cursorName = name_->textCursor();
        Logic::Text2Doc(oldName/*Logic::getContactListModel()->getDisplayName(currentAimId_)*/, cursorName, Logic::Text2DocHtmlMode::Pass, false);

        editLabel_->setVisible(myInfo_);

        bool isChat = Logic::getContactListModel()->isChat(currentAimId_);
        saveButton_->setVisible(isChat);
        saveButtonMargin_->setVisible(isChat);
        name_->setVisible(!isChat);
        avatar_->SetMode(myInfo_ ? ContactAvatarWidget::Mode::MyProfile : ContactAvatarWidget::Mode::Common);
        if (isChat)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_int("limit", Logic::ChatMembersModel::get_limit(members_count));
            Ui::GetDispatcher()->post_message_to_core(qsl("chats/info/get"), collection.get());
        }

        updateStatus();

        bool isIgnored = Logic::getIgnoreModel()->getMemberItem(currentAimId_) != nullptr;
        ignoreWidget_->setVisible(isIgnored);
        buttonWidget_->setVisible(!isIgnored && !isChat && currentAimId_ != MyInfo()->aimId());
        buttonsMargin_->setVisible(!isIgnored && !isChat);

        auto cont = Logic::getContactListModel()->getContactItem(currentAimId_);

        bool isNotAuth = Logic::getContactListModel()->isNotAuth(currentAimId_) || cont == nullptr;
        chatButton_->setVisible(!isNotAuth);
        addButton_->setVisible(isNotAuth);
        callButton_->setEnabled(!isNotAuth);
        videoCall_button_->setEnabled(!isNotAuth);

        callButton_->setVisible(!platform::is_linux());
        videoCall_button_->setVisible(!platform::is_linux());

        ignoreListButton->setVisible(myInfo_);
        renameContact_->setVisible(!isChat && !isNotAuth && !isIgnored && !myInfo_);

        Line_->setVisible((!isChat && !isNotAuth && !isIgnored && !myInfo_) || myInfo_);

        uin_->setInfo(currentAimId_);
        uin_->setVisible(!isChat);

        if (Logic::getContactListModel()->isChat(currentAimId_))
            return;

        Logic::getContactListModel()->getContactProfile(currentAimId_, [this](Logic::profile_ptr _profile, int32_t /*error*/)
        {
            if (!_profile)
                return;

            if (myInfo_)
            {
                if (!MyInfo()->phoneNumber().isEmpty())
                    phone_->setInfo(MyInfo()->phoneNumber(), qsl("+"));

                phone_->setAttachPhone(MyInfo()->phoneNumber().isEmpty());
                phone_->show();
            }
            else
            {
                const auto& list = _profile->get_phones();
                if (!list.empty())
                {
                    phone_->setInfo(list.front().get_phone(), qsl("+"));
                    phone_->show();
                    renameContact_->hide();
                    Line_->hide();
                }
            }

            auto fn = _profile->get_first_name();
            if (!fn.isEmpty())
            {
                firstName_->setInfo(fn);
                firstName_->show();
            }

            auto ln = _profile->get_last_name();
            if (!ln.isEmpty())
            {
                lastName_->setInfo(ln);
                lastName_->show();
            }

            auto nn = myInfo_ ? MyInfo()->friendlyName() : _profile->get_friendly();
            if (!nn.isEmpty())
            {
                nickName_->setInfo(nn);
                nickName_->show();

                if (Logic::getContactListModel()->getDisplayName(currentAimId_) == currentAimId_ || myInfo_)
                {
                    name_->setPlainText(QString());
                    QTextCursor cursorName = name_->textCursor();
                    Logic::Text2Doc(nn, cursorName, Logic::Text2DocHtmlMode::Pass, false);
                    Logic::GetAvatarStorage()->updateAvatar(currentAimId_, false);
                    avatar_->UpdateParams(currentAimId_, nn);
                    avatar_->update();
                }
            }
            else if (Logic::getContactListModel()->getDisplayName(currentAimId_) == currentAimId_)
            {
                const auto space = ql1s(fn.isEmpty() ? "" : " ");
                const QString name = fn % space % ln;
                if (!name.isEmpty())
                {
                    name_->setPlainText(QString());
                    QTextCursor cursorName = name_->textCursor();
                    Logic::Text2Doc(name, cursorName, Logic::Text2DocHtmlMode::Pass, false);
                }
            }

            int64_t birth = _profile->get_birthdate();
            if (birth > 0)
            {
                QDateTime dt = QDateTime::fromTime_t(birth);
                if (dt.isValid())
                {
                    birthday_->setInfo(Utils::GetTranslator()->formatDate(dt.date(), false));
                    birthday_->show();
                }
            }

            auto city = _profile->get_home_address().get_city();
            if (!city.isEmpty())
            {
                city_->setInfo(city);
                city_->show();
            }

            auto countryCode = _profile->get_home_address().get_country().toLower();
            if (!countryCode.isEmpty())
            {
                for (const auto& country : countries::get())
                {
                    if (country.iso_code_ == countryCode)
                    {
                        country_->setInfo(country.name_);
                        country_->show();
                        break;
                    }
                }
            }

            updateWidth();
        });

        updateWidth();

        if (myInfo_)
            connect(MyInfo(), &Ui::my_info::received, this, &ProfilePage::changed, Qt::UniqueConnection);
        else
            disconnect(MyInfo(), &Ui::my_info::received, this, &ProfilePage::changed);
    }

    void ProfilePage::init()
    {
        setStyleSheet(Utils::LoadStyle(qsl(":/qss/sidebar")));
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, this, &ProfilePage::chatInfo, Qt::QueuedConnection);
        connect(GetDispatcher(), &Ui::core_dispatcher::recvFlags, this, &ProfilePage::recvFlags);

        auto layout = Utils::emptyVLayout(this);
        auto area = CreateScrollAreaAndSetTrScrollBarV(this);
        area->horizontalScrollBar()->setDisabled(true);
        layout->addWidget(area);

        area->setContentsMargins(0, 0, 0, 0);
        mainWidget_ = new QWidget(area);
        mainWidget_->setStyleSheet(qsl("background: transparent;"));
        area->setWidget(mainWidget_);
        area->setWidgetResizable(true);
        area->setFrameStyle(QFrame::NoFrame);
        area->setStyleSheet(qsl("background: transparent;"));

        Utils::grabTouchWidget(area->viewport(), true);
        Utils::grabTouchWidget(mainWidget_);

        connect(QScroller::scroller(area->viewport()), &QScroller::stateChanged, this, &ProfilePage::touchScrollStateChanged, Qt::QueuedConnection);

        auto vLayoutMain = Utils::emptyVLayout(mainWidget_);
        vLayoutMain->setContentsMargins(0, Utils::scale_value(HOR_PADDING), 0, 0);

        auto hLayoutMain = Utils::emptyHLayout();
        hLayoutMain->setContentsMargins(Utils::scale_value(HOR_PADDING), 0, 0, 0);

        subBackButtonLayout_ = Utils::emptyVLayout();
        subBackButtonLayout_->setAlignment(Qt::AlignTop);
        hLayoutMain->addLayout(subBackButtonLayout_);

        subAvatarLayout_ = Utils::emptyVLayout();
        subAvatarLayout_->setAlignment(Qt::AlignTop);
        hLayoutMain->addLayout(subAvatarLayout_);

        auto rootLayout = Utils::emptyVLayout();
        rootLayout->setContentsMargins(0, 0, 0, Utils::scale_value(HOR_PADDING));
        rootLayout->setAlignment(Qt::AlignTop);
        {
            auto avatarLayout = Utils::emptyHLayout();
            {
                mainBackButtonLayout_ = Utils::emptyVLayout();
                mainBackButtonLayout_->setAlignment(Qt::AlignTop);
                backButton_ = new CustomButton(mainWidget_, qsl(":/controls/back_100"));
                backButton_->setHoverImage(qsl(":/controls/back_100_hover"));
                backButton_->setFixedSize(Utils::scale_value(28), Utils::scale_value(24));
                backButton_->setCursor(QCursor(Qt::PointingHandCursor));
                Utils::grabTouchWidget(backButton_);
                mainBackButtonLayout_->addWidget(backButton_);
                avatarLayout->addLayout(mainBackButtonLayout_);
            }
            avatarLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            {
                mainAvatarLayout_ = Utils::emptyVLayout();
                mainAvatarLayout_->setAlignment(Qt::AlignTop);
                avatar_ = new ContactAvatarWidget(mainWidget_, QString(), QString(), Utils::scale_value(avatar_size), true);
                auto mainSaveButtonLayout_ = Utils::emptyHLayout();
                mainSaveButtonLayout_->setAlignment(Qt::AlignRight);
                saveButtonSpace_ = new QWidget(mainWidget_);
                Utils::grabTouchWidget(saveButtonSpace_);
                mainSaveButtonLayout_->addWidget(saveButtonSpace_);
                auto subSaveButtonLayout = Utils::emptyVLayout();
                saveButton_ = new QPushButton(mainWidget_);
                Utils::ApplyStyle(saveButton_, CommonStyle::getGreenButtonStyle());
                saveButton_->setText(QT_TRANSLATE_NOOP("sidebar", "SAVE"));
                saveButton_->setCursor(Qt::PointingHandCursor);
                saveButton_->adjustSize();
                Utils::grabTouchWidget(saveButton_);
                subSaveButtonLayout->addWidget(saveButton_);
                saveButtonMargin_ = new QWidget(mainWidget_);
                saveButtonMargin_->setFixedHeight(Utils::scale_value(save_button_offset));
                Utils::grabTouchWidget(saveButtonMargin_);
                subSaveButtonLayout->addWidget(saveButtonMargin_);
                mainSaveButtonLayout_->addLayout(subSaveButtonLayout);
                mainAvatarLayout_->addLayout(mainSaveButtonLayout_);
                Utils::grabTouchWidget(avatar_);
                mainAvatarLayout_->addWidget(avatar_);
                avatarBottomSpace_ = new QWidget(mainWidget_);
                avatarBottomSpace_->setFixedHeight(Utils::scale_value(avatar_botton_spacing));
                Utils::grabTouchWidget(avatarBottomSpace_);
                mainAvatarLayout_->addWidget(avatarBottomSpace_);
                avatarLayout->addLayout(mainAvatarLayout_);
            }
            avatarLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
            editLayout_ = Utils::emptyVLayout();
            editLayout_->setAlignment(Qt::AlignTop);
            editLayout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(edit_top_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
            avatarLayout->addLayout(editLayout_);
            avatarLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(HOR_PADDING), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            rootLayout->addLayout(avatarLayout);

            {
                nameLayout_ = Utils::emptyHLayout();
                nameLayout_->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, Utils::scale_value(HOR_PADDING), 0);
                name_ = new TextEditEx(mainWidget_, Fonts::appFontScaled(18, Fonts::FontWeight::Medium), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY), false, false);
                name_->setFrameStyle(QFrame::NoFrame);
                name_->setContentsMargins(0, 0, 0, 0);
                name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setContextMenuPolicy(Qt::NoContextMenu);
                Utils::grabTouchWidget(name_);
                nameLayout_->addWidget(name_);
                subEditLayout_ = Utils::emptyVLayout();
                subEditLayout_->setContentsMargins(0, Utils::scale_value(edit_top_margin), 0, 0);
                subEditLayout_->setAlignment(Qt::AlignTop);
                editLabel_ = new LabelEx(mainWidget_);
                editLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Edit"));
                editLabel_->setProperty("edit", true);
                editLabel_->setColor(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
                editLabel_->setFont(Fonts::appFontScaled(14));
                editLabel_->setCursor(Qt::PointingHandCursor);
                editLabel_->adjustSize();
                subEditLayout_->addWidget(editLabel_);
                Utils::grabTouchWidget(editLabel_);
                nameLayout_->addSpacerItem(new QSpacerItem(Utils::scale_value(HOR_PADDING), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                nameLayout_->addLayout(subEditLayout_);
                nameLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                rootLayout->addLayout(nameLayout_);
            }
            {
                auto horLayoutIn = Utils::emptyHLayout();
                horLayoutIn->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, 0, 0);

                statusLabel_ = new LabelEx(mainWidget_);
                statusLabel_->setFont(Fonts::appFontScaled(14));
                statusLabel_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));

                Utils::grabTouchWidget(statusLabel_);
                horLayoutIn->addWidget(statusLabel_);
                horLayoutIn->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                rootLayout->addLayout(horLayoutIn);
            }

            buttonsMargin_ = new QWidget(mainWidget_);
            buttonsMargin_->setFixedHeight(Utils::scale_value(buttons_top_margin));
            Utils::grabTouchWidget(buttonsMargin_);
            rootLayout->addWidget(buttonsMargin_);


            {
                auto horLayoutIn = Utils::emptyHLayout();
                horLayoutIn->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, Utils::scale_value(HOR_PADDING), 0);
                ignoreWidget_ = new QWidget(mainWidget_);
                ignoreWidget_->setStyleSheet(qsl("background-color: #fbdbd9;"));
                {
                    auto ignoreLayout = Utils::emptyVLayout(ignoreWidget_);
                    {
                        auto vLayout = new QVBoxLayout();
                        vLayout->setSpacing(Utils::scale_value(4));
                        vLayout->setContentsMargins(Utils::scale_value(back_button_spacing), Utils::scale_value(buttons_margin), Utils::scale_value(back_button_spacing), Utils::scale_value(buttons_margin));
                        auto ignoreText = new LabelEx(ignoreWidget_);
                        ignoreText->setWordWrap(true);
                        ignoreText->setFont(Fonts::appFontScaled(16));
                        ignoreText->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
                        ignoreText->setText(QT_TRANSLATE_NOOP("sidebar", "This contact is in the ignore list"));
                        vLayout->addWidget(ignoreText);
                        ignoreDelete_ = new LabelEx(ignoreWidget_);
                        ignoreDelete_->setFont(Fonts::appFontScaled(16));
                        ignoreDelete_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_RED));
                        ignoreDelete_->setText(QT_TRANSLATE_NOOP("sidebar", "Delete"));
                        ignoreDelete_->setCursor(QCursor(Qt::PointingHandCursor));
                        vLayout->addWidget(ignoreDelete_);
                        ignoreLayout->addLayout(vLayout);
                    }
                }
                Utils::grabTouchWidget(ignoreDelete_);
                Utils::grabTouchWidget(ignoreWidget_);
                horLayoutIn->addWidget(ignoreWidget_);
                horLayoutIn->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
                rootLayout->addLayout(horLayoutIn);
            }

            {
                buttonWidget_ = new QWidget(mainWidget_);
                auto hLayout = new QHBoxLayout(buttonWidget_);
                hLayout->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, 0, Utils::scale_value(buttons_margin));
                hLayout->setSpacing(Utils::scale_value(buttons_spacing));
                addButton_ = new CustomButton(mainWidget_, qsl(":/resources/profile_page/addpeople_100.png"));
                addButton_->setHoverImage(qsl(":/resources/profile_page/addpeople_100_hover.png"));
                addButton_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                addButton_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(addButton_);

                chatButton_ = new CustomButton(mainWidget_, qsl(":/resources/profile_page/writemsg_100.png"));
                chatButton_->setHoverImage(qsl(":/resources/profile_page/writemsg_100_hover.png"));
                chatButton_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                chatButton_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(chatButton_);

                callButton_ = new CustomButton(mainWidget_, qsl(":/resources/profile_page/call_big_100.png"));
                callButton_->setHoverImage(qsl(":/resources/profile_page/call_big_100_hover.png"));
                callButton_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                callButton_->setDisabledImage(qsl(":/resources/profile_page/call_bigdisable_100.png"));
                callButton_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(callButton_);

                videoCall_button_ = new CustomButton(mainWidget_, qsl(":/resources/profile_page/videocall_big_100.png"));
                videoCall_button_->setHoverImage(qsl(":/resources/profile_page/videocall_big_100_hover.png"));
                videoCall_button_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                videoCall_button_->setDisabledImage(qsl(":/resources/profile_page/videocall_bigdisable_100.png"));
                videoCall_button_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(videoCall_button_);

                hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
            }
            Utils::grabTouchWidget(addButton_);
            Utils::grabTouchWidget(chatButton_);
            Utils::grabTouchWidget(callButton_);
            Utils::grabTouchWidget(videoCall_button_);
            Utils::grabTouchWidget(buttonWidget_);
            rootLayout->addWidget(buttonWidget_);

            shareLink_ = new ActionButton(mainWidget_, qsl(":/resources/i_share_100.png"), QT_TRANSLATE_NOOP("sidebar", "Share contact"), Utils::scale_value(button_height), 0, Utils::scale_value(HOR_PADDING));
            shareLink_->setCursor(QCursor(Qt::PointingHandCursor));
            Utils::grabTouchWidget(shareLink_);
            rootLayout->addWidget(shareLink_);

            uin_ = new InfoPlate(mainWidget_, Utils::scale_value(LEFT_MARGIN));
            uin_->setHeader(build::is_icq() ?
                QT_TRANSLATE_NOOP("sidebar", "UIN")
                : QT_TRANSLATE_NOOP("sidebar", "Email/UIN")
            );
            rootLayout->addWidget(uin_);

            phone_ = new InfoPlate(mainWidget_, Utils::scale_value(LEFT_MARGIN));
            phone_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Phone number"));
            rootLayout->addWidget(phone_);

            firstName_ = new InfoPlate(mainWidget_, Utils::scale_value(LEFT_MARGIN));
            firstName_->setHeader(QT_TRANSLATE_NOOP("sidebar", "First name"));
            rootLayout->addWidget(firstName_);

            lastName_ = new InfoPlate(mainWidget_, Utils::scale_value(LEFT_MARGIN));
            lastName_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Last name"));
            rootLayout->addWidget(lastName_);

            nickName_ = new InfoPlate(mainWidget_, Utils::scale_value(LEFT_MARGIN));
            nickName_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Nickname"));
            rootLayout->addWidget(nickName_);

            birthday_ = new InfoPlate(mainWidget_, Utils::scale_value(LEFT_MARGIN));
            birthday_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Birthday"));
            rootLayout->addWidget(birthday_);

            city_ = new InfoPlate(mainWidget_, Utils::scale_value(LEFT_MARGIN));
            city_->setHeader(QT_TRANSLATE_NOOP("sidebar", "City"));
            rootLayout->addWidget(city_);

            country_ = new InfoPlate(mainWidget_, Utils::scale_value(LEFT_MARGIN));
            country_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Country"));
            rootLayout->addWidget(country_);

            chatEditWidget_ = new QWidget(mainWidget_);
            auto vLayout = Utils::emptyVLayout(chatEditWidget_);
            {
                auto hLayout = Utils::emptyHLayout();
                hLayout->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, 0, 0);
                hLayout->setAlignment(Qt::AlignLeft);
                auto labelName = new LabelEx(chatEditWidget_);
                labelName->setText(QT_TRANSLATE_NOOP("sidebar", "NAME"));
                labelName->setFont(Fonts::appFontScaled(12, Fonts::FontWeight::Medium));
                labelName->setColor(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
                hLayout->addWidget(labelName);
                vLayout->addLayout(hLayout);
            }

            {
                auto hLayout = Utils::emptyHLayout();
                hLayout->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, 0, 0);
                hLayout->setAlignment(Qt::AlignLeft);
                nameEdit_ = new Ui::TextEditEx(chatEditWidget_, Fonts::appFontScaled(18), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY), true, true);
                Utils::ApplyStyle(nameEdit_, CommonStyle::getTextEditStyle());
                nameEdit_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
                nameEdit_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                nameEdit_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
                nameEdit_->setCatchEnter(false);
                nameEdit_->setFrameStyle(QFrame::NoFrame);
                nameEdit_->document()->setDocumentMargin(0);
                nameEdit_->addSpace(Utils::scale_value(4));
                hLayout->addWidget(nameEdit_);
                vLayout->addLayout(hLayout);
            }

            vLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(name_desc_space), QSizePolicy::Preferred, QSizePolicy::Fixed));

            {
                auto hLayout = Utils::emptyHLayout();
                hLayout->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, 0, 0);
                hLayout->setAlignment(Qt::AlignLeft);
                auto labelDescription = new LabelEx(chatEditWidget_);
                labelDescription->setText(QT_TRANSLATE_NOOP("sidebar", "DESCRIPTION"));
                labelDescription->setFont(Fonts::appFontScaled(12, Fonts::FontWeight::Medium));
                labelDescription->setColor(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
                hLayout->addWidget(labelDescription);
                vLayout->addLayout(hLayout);
            }

            {
                auto hLayout = Utils::emptyHLayout();
                hLayout->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, 0, 0);
                hLayout->setAlignment(Qt::AlignLeft);
                descriptionEdit_ = new Ui::TextEditEx(chatEditWidget_, Fonts::appFontScaled(18), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY), true, true);
                Utils::ApplyStyle(descriptionEdit_, CommonStyle::getTextEditStyle());
                descriptionEdit_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
                descriptionEdit_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                descriptionEdit_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
                descriptionEdit_->setCatchEnter(false);
                descriptionEdit_->setFrameStyle(QFrame::NoFrame);
                descriptionEdit_->document()->setDocumentMargin(0);
                descriptionEdit_->addSpace(Utils::scale_value(4));
                hLayout->addWidget(descriptionEdit_);
                vLayout->addLayout(hLayout);
            }

            Utils::grabTouchWidget(descriptionEdit_);
            Utils::grabTouchWidget(nameEdit_);
            Utils::grabTouchWidget(chatEditWidget_);
            rootLayout->addWidget(chatEditWidget_);

            Line_ = new LineWidget(mainWidget_, Utils::scale_value(LEFT_MARGIN), 0, Utils::scale_value(HOR_PADDING), 0);
            Utils::grabTouchWidget(Line_);
            rootLayout->addWidget(Line_);

            renameContact_ = new ActionButton(mainWidget_, qsl(":/resources/profile_page/profile_rename_100.png"), QT_TRANSLATE_NOOP("sidebar", "Rename"), Utils::scale_value(button_height), 0, Utils::scale_value(HOR_PADDING));
            renameContact_->setCursor(QCursor(Qt::PointingHandCursor));
            Utils::grabTouchWidget(renameContact_);
            rootLayout->addWidget(renameContact_);

            ignoreListButton = new ActionButton(mainWidget_, qsl(":/resources/i_ignore_100.png"), QT_TRANSLATE_NOOP("sidebar", "Ignored contacts"), Utils::scale_value(button_height), 0, Utils::scale_value(HOR_PADDING));
            ignoreListButton->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_RED).name());
            ignoreListButton->setCursor(QCursor(Qt::PointingHandCursor));
            Utils::grabTouchWidget(ignoreListButton);
            rootLayout->addWidget(ignoreListButton);

            attachOldAcc = new ActionButton(mainWidget_, qsl(":/resources/profile_page/profile_oldaccount_100.png"), QT_TRANSLATE_NOOP("sidebar", "Connect to ICQ account"), Utils::scale_value(button_height), 0, Utils::scale_value(HOR_PADDING));
            attachOldAcc->setColor(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT).name());
            attachOldAcc->setCursor(QCursor(Qt::PointingHandCursor));
            Utils::grabTouchWidget(attachOldAcc);
            rootLayout->addWidget(attachOldAcc);

            rootLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
            hLayoutMain->addLayout(rootLayout);
            vLayoutMain->addLayout(hLayoutMain);

        }

        connect(ignoreDelete_, &Ui::LabelEx::clicked, this, &ProfilePage::removeFromIgnore, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &ProfilePage::contactChanged, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &ProfilePage::contactRemoved, Qt::QueuedConnection);

        connect(addButton_, &Ui::CustomButton::clicked, this, &ProfilePage::addClicked, Qt::QueuedConnection);
        connect(chatButton_, &Ui::CustomButton::clicked, this, &ProfilePage::chatClicked, Qt::QueuedConnection);
        connect(callButton_, &Ui::CustomButton::clicked, this, &ProfilePage::callClicked, Qt::QueuedConnection);
        connect(videoCall_button_, &Ui::CustomButton::clicked, this, &ProfilePage::videoClicked, Qt::QueuedConnection);
        connect(backButton_, &Ui::CustomButton::clicked, this, &ProfilePage::back, Qt::QueuedConnection);

        connect(shareLink_, &ActionButton::clicked, this, &ProfilePage::copyLinkClicked, Qt::QueuedConnection);

        connect(renameContact_, &ActionButton::clicked, this, &ProfilePage::rename, Qt::QueuedConnection);
        connect(ignoreListButton, &ActionButton::clicked, this, &ProfilePage::ignoreList, Qt::QueuedConnection);
        connect(attachOldAcc, &ActionButton::clicked, this, &ProfilePage::attachOld, Qt::QueuedConnection);
        connect(editLabel_, &Ui::LabelEx::clicked, this, &ProfilePage::editClicked, Qt::QueuedConnection);
        connect(saveButton_, &QPushButton::clicked, this, &ProfilePage::saveClicked, Qt::QueuedConnection);

        connect(avatar_, &Ui::ContactAvatarWidget::afterAvatarChanged, this, &Ui::ProfilePage::avatarChanged, Qt::QueuedConnection);
    }

    void ProfilePage::chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>& info)
    {
        if (info->AimId_ == currentAimId_)
        {
            info_ = info;
            if (info_->Controlled_ == false || info_->YourRole_ == ql1s("admin"))
            {
                avatar_->SetMode(ContactAvatarWidget::Mode::MyProfile);
                if (QApplication::focusWidget() != nameEdit_)
                {
                    nameEdit_->setPlaceholderText(info->Name_);
                    nameEdit_->setPlainText(info->Name_);
                }
                if (QApplication::focusWidget() != descriptionEdit_)
                {
                    descriptionEdit_->setPlaceholderText(info->About_.isEmpty() ? QT_TRANSLATE_NOOP("sidebar", "Add your description") : info_->About_);
                    descriptionEdit_->setPlainText(info->About_);
                }
                chatEditWidget_->show();
            }
            updateWidth();
        }
    }

    void ProfilePage::removeFromIgnore()
    {
        deleteMemberDialog(Logic::getIgnoreModel(), currentAimId_, Logic::IGNORE_LIST, this);
    }

    void ProfilePage::contactChanged(const QString& aimId)
    {
        if (aimId == currentAimId_)
            initFor(MyInfo()->aimId() == aimId ? QString() : aimId);
    }

    void ProfilePage::contactRemoved(const QString& aimId)
    {
        if (aimId == currentAimId_)
            Utils::InterConnector::instance().setSidebarVisible(false);
    }

    void ProfilePage::changed()
    {
        initFor(QString());
    }

    void ProfilePage::updateStatus()
    {
        statusLabel_->hide();
        if (Logic::getContactListModel()->isChat(currentAimId_))
            return;

        QString state;
        if (myInfo_)
        {
            state = MyInfo()->state();
        }
        else
        {
            state = Logic::getContactListModel()->getStatusString(currentAimId_);
        }

        statusLabel_->setText(state);

        if (!state.isEmpty())
        {
            statusLabel_->show();
        }
    }

    void ProfilePage::addClicked()
    {
        emit Utils::InterConnector::instance().closeAnySemitransparentWindow();

        Logic::getContactListModel()->addContactToCL(currentAimId_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_profile_page);
    }

    void ProfilePage::chatClicked()
    {
        emit Utils::InterConnector::instance().closeAnySemitransparentWindow();

        const auto selectedContact = Logic::getContactListModel()->selectedContact();
        if (selectedContact != currentAimId_)
            emit Utils::InterConnector::instance().addPageToDialogHistory(selectedContact);

        Logic::getContactListModel()->setCurrent(currentAimId_, -1, true);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_write_message);
    }

    void ProfilePage::callClicked()
    {
        emit Utils::InterConnector::instance().closeAnySemitransparentWindow();

        Ui::GetDispatcher()->getVoipController().setStartA(currentAimId_.toUtf8(), false);
        if (MainPage* mainPage = MainPage::instance()) {
            mainPage->raiseVideoWindow();
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_call);
    }

    void ProfilePage::videoClicked()
    {
        emit Utils::InterConnector::instance().closeAnySemitransparentWindow();

        Ui::GetDispatcher()->getVoipController().setStartV(currentAimId_.toUtf8(), false);
        if (MainPage* mainPage = MainPage::instance()) {
            mainPage->raiseVideoWindow();
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_video_call);
    }

    void ProfilePage::back()
    {
        if (myInfo_)
        {
            emit Utils::InterConnector::instance().myProfileBack();
            return;
        }

        if (prevAimId_.isEmpty())
            Utils::InterConnector::instance().setSidebarVisible(false);
        else
            Utils::InterConnector::instance().showSidebar(prevAimId_, menu_page);
    }

    void ProfilePage::rename()
    {
        QString result_chat_name;

        bool isChat = Logic::getContactListModel()->isChat(currentAimId_);

        auto result = Utils::NameEditor(
            this,
            Logic::getContactListModel()->getDisplayName(currentAimId_),
            QT_TRANSLATE_NOOP("sidebar","SAVE"),
            isChat ? QT_TRANSLATE_NOOP("sidebar", "Chat name") : QT_TRANSLATE_NOOP("sidebar", "Contact name"),
            result_chat_name);

        if (result)
            Logic::getContactListModel()->renameContact(currentAimId_, result_chat_name);
    }

    void ProfilePage::resizeEvent(QResizeEvent* e)
    {
        mainWidget_->setFixedWidth(e->size().width());
        if (e->size().width() >= Utils::scale_value(596))
        {
            mainBackButtonLayout_->takeAt(0);
            if (mainAvatarLayout_->itemAt(1) && mainAvatarLayout_->itemAt(1)->widget() == avatar_)
                mainAvatarLayout_->takeAt(1);
            avatarBottomSpace_->setVisible(false);
            subBackButtonLayout_->insertWidget(0, backButton_);
            subAvatarLayout_->insertWidget(0, avatar_);
            if (editLayout_->itemAt(1) && editLayout_->itemAt(1)->widget() && editLayout_->itemAt(1)->widget()->property("edit").toBool())
                editLayout_->takeAt(1);
            subEditLayout_->insertWidget(1, editLabel_);
            saveButtonMargin_->hide();
        }
        else
        {
            subBackButtonLayout_->takeAt(0);
            subAvatarLayout_->takeAt(0);
            avatarBottomSpace_->setVisible(true);
            mainBackButtonLayout_->insertWidget(0, backButton_);
            mainAvatarLayout_->insertWidget(1, avatar_);
            if (subEditLayout_->itemAt(1) && subEditLayout_->itemAt(1)->widget() && subEditLayout_->itemAt(1)->widget()->property("edit").toBool())
                subEditLayout_->takeAt(1);
            editLayout_->insertWidget(1, editLabel_);
            saveButtonMargin_->setVisible(Logic::getContactListModel()->isChat(currentAimId_));
        }
        QWidget::resizeEvent(e);
        updateWidth();
    }

    void ProfilePage::ignoreList()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignorelist_open);
        QVector<QString> temp;
        Logic::updateIgnoredModel(temp);

        SelectContactsWidget select_members_dialog_(
            Logic::getIgnoreModel(),
            Logic::MembersWidgetRegim::IGNORE_LIST,
            QT_TRANSLATE_NOOP("popup_window", "Ignored contacts"),
            QT_TRANSLATE_NOOP("popup_window", "DONE"),
            this);
        connect(GetDispatcher(), &core_dispatcher::recvPermitDeny, &select_members_dialog_, &SelectContactsWidget::UpdateViewForIgnoreList, Qt::QueuedConnection);

        Logic::ContactListModel::getIgnoreList();
        select_members_dialog_.setView(false);
        select_members_dialog_.show(-1, -1);
    }

    void ProfilePage::editClicked()
    {
        if (build::is_icq())
        {
            const auto url = qsl("https://icq.com/people/%1/edit").arg(currentAimId_);
            core::coll_helper helper(GetDispatcher()->create_collection(), true);
            helper.set_value_as_string("url", url.toStdString());
            GetDispatcher()->post_message_to_core(qsl("sign_url"), helper.get());
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::myprofile_edit);

            disconnect(GetDispatcher(), &core_dispatcher::signedUrl, nullptr, nullptr);
            connect(GetDispatcher(), &core_dispatcher::signedUrl, this, [](const QString& url)
            {
                QDesktopServices::openUrl(url);
            });
        }
        else
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

            collection.set_value_as_qstring("email", currentAimId_);

            Ui::GetDispatcher()->post_message_to_core(qsl("mrim/get_key"), collection.get(), this, [this](core::icollection* _collection)
            {
                const QString profileUrl = qsl("https://e.mail.ru/settings/userinfo");

                Utils::openAgentUrl(profileUrl, profileUrl, currentAimId_, QString::fromUtf8(Ui::gui_coll_helper(_collection, false).get_value_as_string("key")));
            });
        }
    }

    void ProfilePage::saveClicked()
    {
        if (info_)
        {
            auto name = nameEdit_->getPlainText();
            if (name != info_->Name_ && !name.isEmpty())
            {
                if (Logic::getContactListModel()->isLiveChat(currentAimId_))
                {
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("aimid", currentAimId_);
                    collection.set_value_as_qstring("name", name);
                    Ui::GetDispatcher()->post_message_to_core(qsl("chats/mod/name"), collection.get());
                }
                else
                {
                    Logic::getContactListModel()->renameChat(currentAimId_, name);
                }
            }
            auto desc = descriptionEdit_->getPlainText();
            if (desc != info_->About_)
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("aimid", currentAimId_);
                collection.set_value_as_qstring("about", desc);
                Ui::GetDispatcher()->post_message_to_core(qsl("chats/mod/about"), collection.get());
            }
        }
        back();
    }

    void ProfilePage::attachOld()
    {
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_AttachUin);
    }

    void ProfilePage::recvFlags(int flags)
    {
        connectOldVisible_ = flags & 0x1000;
        attachOldAcc->setVisible(myInfo_ && connectOldVisible_);
    }

    void ProfilePage::avatarChanged()
    {
        if (Logic::getContactListModel()->isChat(currentAimId_))
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_avatar_changed);
        }
        else
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_avatar_changed);
        }
    }
}
