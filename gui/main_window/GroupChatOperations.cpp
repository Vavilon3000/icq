#include "stdafx.h"

#include "GroupChatOperations.h"
#include "../core_dispatcher.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/ChatMembersModel.h"
#include "../utils/utils.h"
#include "../my_info.h"
#include "../utils/gui_coll_helper.h"
#include "MainPage.h"
#include "../gui_settings.h"
#include "contact_list/ContactList.h"
#include "contact_list/SelectionContactsForGroupChat.h"

#include "history_control/MessageStyle.h"
#include "../utils/InterConnector.h"
#include "../controls/CommonStyle.h"
#include "../controls/GeneralDialog.h"
#include "../controls/TextEditEx.h"
#include "../controls/ContactAvatarWidget.h"
#include "MainWindow.h"

namespace Ui
{
    GroupChatSettings::GroupChatSettings(QWidget *parent, const QString &buttonText, const QString &headerText, GroupChatOperations::ChatData &chatData)
        : chatData_(chatData), editorIsShown_(false)
    {
        content_ = new QWidget(parent);
        content_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        content_->setFixedWidth(Utils::scale_value(360));
        Utils::ApplyStyle(content_, qsl("height: 10dip;"));

        auto layout = Utils::emptyVLayout(content_);
        layout->setContentsMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(16), 0);

        // name and photo
        auto subContent = new QWidget(content_);
        subContent->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        auto subLayout = new QHBoxLayout(subContent);
        subLayout->setSpacing(Utils::scale_value(16));
        subLayout->setContentsMargins(0, 0, 0, Utils::scale_value(12));
        subLayout->setAlignment(Qt::AlignTop);
        {
            photo_ = new ContactAvatarWidget(subContent, QString(), QString(), Utils::scale_value(56), true);
            photo_->SetMode(ContactAvatarWidget::Mode::CreateChat);
            photo_->SetVisibleShadow(false);
            photo_->SetVisibleSpinner(false);
            subLayout->addWidget(photo_);

            chatName_ = new TextEditEx(subContent, Fonts::appFontScaled(18), MessageStyle::getTextColor(), true, true);
            Utils::ApplyStyle(chatName_, CommonStyle::getTextEditStyle());
            chatName_->setWordWrapMode(QTextOption::NoWrap);//WrapAnywhere);
            chatName_->setObjectName(qsl("chat_name"));
            chatName_->setPlaceholderText(QT_TRANSLATE_NOOP("groupchats", "Chat name"));
            chatName_->setPlainText(QString());//chatData.name);
            chatName_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            chatName_->setAutoFillBackground(false);
            chatName_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            chatName_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            chatName_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
            chatName_->setCatchEnter(true);
            chatName_->setMinimumWidth(Utils::scale_value(200));
            chatName_->document()->setDocumentMargin(0);
            chatName_->addSpace(Utils::scale_value(4));
            subLayout->addWidget(chatName_);
        }
        layout->addWidget(subContent);

        // settings
        static auto createItem = [](QWidget *parent, const QString &topic, const QString &text, QCheckBox *&switcher)
        {
            auto whole = new QWidget(parent);
            auto wholelayout = new QHBoxLayout(whole);
            whole->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
            wholelayout->setContentsMargins(0, 0, 0, Utils::scale_value(12));
            {
                auto textpart = new QWidget(whole);
                auto textpartlayout = Utils::emptyVLayout(textpart);
                textpart->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
                textpartlayout->setAlignment(Qt::AlignTop);
                Utils::ApplyStyle(textpart, qsl("background-color: transparent; border: none;"));
                {
                    {
                        auto t = new TextEmojiWidget(textpart, Fonts::appFontScaled(16), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY), Utils::scale_value(16));
                        t->setText(topic);
                        t->setObjectName(qsl("topic"));
                        t->setContentsMargins(0, 0, 0, 0);
                        t->setContextMenuPolicy(Qt::NoContextMenu);
                        textpartlayout->addWidget(t);
                    }
                    {
                        auto t = new TextEmojiWidget(textpart, Fonts::appFontScaled(13), CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT), Utils::scale_value(16));
                        t->setText(text);
                        t->setObjectName(qsl("text"));
                        t->setContentsMargins(0, 0, 0, 0);
                        t->setContextMenuPolicy(Qt::NoContextMenu);
                        t->setMultiline(true);
                        textpartlayout->addWidget(t);
                    }
                }
                wholelayout->addWidget(textpart);

                auto switchpart = new QWidget(whole);
                auto switchpartlayout = Utils::emptyVLayout(switchpart);
                switchpart->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
                switchpartlayout->setAlignment(Qt::AlignTop);
                Utils::ApplyStyle(switchpart, qsl("background-color: transparent;"));
                {
                    switcher = new QCheckBox(switchpart);
                    switcher->setObjectName(qsl("greenSwitcher"));
                    switcher->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                    switcher->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
                    switcher->setChecked(false);
                    switchpartlayout->addWidget(switcher);
                }
                wholelayout->addWidget(switchpart);
            }
            return whole;
        };
        QCheckBox *approvedJoin;
        layout->addWidget(createItem(content_,
            QT_TRANSLATE_NOOP("groupchats", "Join with Approval"),
            QT_TRANSLATE_NOOP("groupchats", "New members are waiting for admin approval"),
            approvedJoin));
        approvedJoin->setChecked(chatData.approvedJoin);
        connect(approvedJoin, &QCheckBox::toggled, approvedJoin, [&chatData](bool checked){ chatData.approvedJoin = checked; });

        QCheckBox *readOnly;
        layout->addWidget(createItem(content_,
            QT_TRANSLATE_NOOP("groupchats", "Read only"),
            QT_TRANSLATE_NOOP("groupchats", "New members can read, approved by admin members can send"),
            readOnly));
        readOnly->setChecked(chatData.readOnly);
        connect(readOnly, &QCheckBox::toggled, readOnly, [&chatData](bool checked){ chatData.readOnly = checked; });

        QCheckBox *publicChat;
        layout->addWidget(createItem(content_,
            QT_TRANSLATE_NOOP("groupchats", "Public chat"),
            QT_TRANSLATE_NOOP("groupchats", "The chat will appear in the app's showcase and any user can find it in the list"),
            publicChat));
        publicChat->setChecked(chatData.publicChat);
        connect(publicChat, &QCheckBox::toggled, publicChat, [&chatData](bool checked) { chatData.publicChat = checked; });


        // general dialog
        dialog_ = std::make_unique<Ui::GeneralDialog>(content_, Utils::InterConnector::instance().getMainWindow());
        dialog_->setObjectName(qsl("chat.creation.settings"));
        dialog_->addHead();
        dialog_->addLabel(headerText);
        dialog_->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), buttonText, true);

        QObject::connect(chatName_, &TextEditEx::enter, dialog_.get(), &Ui::GeneralDialog::accept);

        QTextCursor cursor = chatName_->textCursor();
        cursor.select(QTextCursor::Document);
        chatName_->setTextCursor(cursor);
        chatName_->setFrameStyle(QFrame::NoFrame);

        chatName_->setFocus();

        // editor
        auto hollow = new QWidget(dialog_->parentWidget());
        hollow->setObjectName(qsl("hollow"));
        hollow->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        Utils::ApplyStyle(hollow, qsl("background-color: transparent; margin: 0; padding: 0; border: none;"));
        {
            auto hollowlayout = Utils::emptyVLayout(hollow);
            hollowlayout->setAlignment(Qt::AlignLeft);

            auto editor = new QWidget(hollow);
            editor->setObjectName(qsl("editor"));
            editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            Utils::ApplyStyle(editor, qsl("background-color: #fffff; margin: 0; padding: 0dip; border: none;"));
            photo_->SetImageCropHolder(editor);
            {
                auto effect = new QGraphicsOpacityEffect(editor);
                effect->setOpacity(.0);
                editor->setGraphicsEffect(effect);
            }
            hollowlayout->addWidget(editor);

            // events

            QWidget::connect(photo_, &ContactAvatarWidget::avatarDidEdit, this, [=]()
            {
                lastCroppedImage_ = photo_->croppedImage();
            }, Qt::QueuedConnection);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::imageCropDialogIsShown, this, [=](QWidget *p)
            {
                if (!editorIsShown_)
                    showImageCropDialog();
                editorIsShown_ = true;
            }, Qt::QueuedConnection);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::imageCropDialogIsHidden, this, [=](QWidget *p)
            {
                if (editorIsShown_)
                    showImageCropDialog();
                editorIsShown_ = false;
            }, Qt::QueuedConnection);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::imageCropDialogResized, this, [=](QWidget *p)
            {
                editor->setFixedSize(p->size());
                p->move(0, 0);
            }, Qt::QueuedConnection);

            connect(dialog_.get(), &GeneralDialog::moved, this, [=](QWidget *p)
            {
                hollow->move(dialog_->x() + dialog_->width(), dialog_->y());
            }, Qt::QueuedConnection);
            connect(dialog_.get(), &GeneralDialog::resized, this, [=](QWidget *p)
            {
                hollow->setFixedSize(hollow->parentWidget()->width() - dialog_->x() + dialog_->width(), dialog_->height());
                const auto preferredWidth = (dialog_->parentWidget()->geometry().size().width() - dialog_->geometry().size().width() - Utils::scale_value(24)*4);
                photo_->SetImageCropSize(QSize(preferredWidth, content_->height()));
            }, Qt::QueuedConnection);
            connect(dialog_.get(), &GeneralDialog::hidden, this, [=](QWidget *p)
            {
                emit Utils::InterConnector::instance().closeAnyPopupWindow();
                emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
            }, Qt::QueuedConnection);
        }
        hollow->show();
    }

    GroupChatSettings::~GroupChatSettings()
    {
        //
    }

    bool GroupChatSettings::show()
    {
        if (!dialog_)
        {
            return false;
        }
        auto result = dialog_->showInPosition(-1, -1);
        if (!chatName_->getPlainText().isEmpty())
            chatData_.name = chatName_->getPlainText();
        return result;
    }

    void GroupChatSettings::showImageCropDialog()
    {
        auto editor = dialog_->parentWidget()->findChild<QWidget *>(qsl("editor"));
        if (!editor)
            return;
        auto hollow = dialog_->parentWidget()->findChild<QWidget *>(qsl("hollow"));
        if (!hollow)
            return;

        static const auto time = 200;
        const auto needHideEditor = editorIsShown_;
        editorIsShown_ = !editorIsShown_;
        // move settings
        {
            auto geometry = new QPropertyAnimation(dialog_.get(), QByteArrayLiteral("geometry"), Utils::InterConnector::instance().getMainWindow());
            geometry->setDuration(time);
            auto rect = dialog_->geometry();
            geometry->setStartValue(rect);
            if (needHideEditor)
            {
                const auto screenRect = Utils::InterConnector::instance().getMainWindow()->geometry();
                const auto dialogRect = dialog_->geometry();
                rect.setX((screenRect.size().width() - dialogRect.size().width()) / 2);
            }
            else
            {
                const auto screenRect = Utils::InterConnector::instance().getMainWindow()->geometry();
                const auto dialogRect = dialog_->geometry();
                const auto editorRect = editor->geometry();
                const auto hollowRect = hollow->geometry();
                const auto between = (hollowRect.left() - dialogRect.right());
//                rect.setX((screenRect.size().width() - dialogRect.size().width() - editorRect.size().width() - Utils::scale_value(24)) / 2);
                rect.setX((screenRect.size().width() - dialogRect.size().width() - editorRect.size().width()) / 2 - between);
            }
            geometry->setEndValue(rect);
            geometry->start(QAbstractAnimation::DeleteWhenStopped);
        }
        // fade editor
        {
            auto effect = new QGraphicsOpacityEffect(Utils::InterConnector::instance().getMainWindow());
            editor->setGraphicsEffect(effect);
            auto fading = new QPropertyAnimation(effect, QByteArrayLiteral("opacity"), Utils::InterConnector::instance().getMainWindow());
            fading->setEasingCurve(QEasingCurve::InOutQuad);
            fading->setDuration(time);
            if (needHideEditor)
            {
                fading->setStartValue(1.0);
                fading->setEndValue(0.0);
                fading->connect(fading, &QPropertyAnimation::finished, fading, [editor, hollow]() { editor->setFixedSize(hollow->size()); });
            }
            else
            {
                fading->setStartValue(0.0);
                fading->setEndValue(1.0);
                fading->connect(fading, &QPropertyAnimation::finished, fading, [editor]() { Utils::addShadowToWidget(editor); });
            }
            fading->start(QPropertyAnimation::DeleteWhenStopped);
        }
    }

    void createGroupChat(const std::vector<QString>& _members_aimIds)
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_open);

        SelectContactsWidget select_members_dialog(nullptr, Logic::MembersWidgetRegim::SELECT_MEMBERS, QT_TRANSLATE_NOOP("groupchats", "Select members"), QT_TRANSLATE_NOOP("popup_window", "NEXT"), Ui::MainPage::instance());
        emit Utils::InterConnector::instance().searchEnd();

        for (const auto& member_aimId : _members_aimIds)
            Logic::getContactListModel()->setChecked(member_aimId, true /* is_checked */);

        if (select_members_dialog.show() == QDialog::Accepted)
        {
            //select_members_dialog.close();
//            if (Logic::getContactListModel()->GetCheckedContacts().size() == 1)
//            {
//                const auto& aimid = Logic::getContactListModel()->GetCheckedContacts()[0].get_aimid();
//                Ui::MainPage::instance()->selectRecentChat(aimid);
//            }
//            else
            {
                std::shared_ptr<GroupChatSettings> groupChatSettings;
                GroupChatOperations::ChatData chatData;
                if (callChatNameEditor(Ui::MainPage::instance(), chatData, groupChatSettings))
                {
                    const auto cropped = groupChatSettings->lastCroppedImage();
                    if (!cropped.isNull())
                    {
                        std::shared_ptr<QMetaObject::Connection> connection(new QMetaObject::Connection());
                        *connection = QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setAvatarId, [connection, chatData](QString avatarId)
                        {
                            if (!avatarId.isEmpty())
                                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_avatar);
                            Ui::MainPage::instance()->openCreatedGroupChat();
                            postCreateChatInfoToCore(MyInfo()->aimId(), chatData, avatarId);
                            Logic::getContactListModel()->clearChecked();
                            Ui::MainPage::instance()->clearSearchMembers();
                            QWidget::disconnect(*connection);
                        });
                        groupChatSettings->photo()->UpdateParams(QString(), QString());
                        groupChatSettings->photo()->applyAvatar(cropped);
                    }
                    else
                    {
                        Ui::MainPage::instance()->openCreatedGroupChat();
                        postCreateChatInfoToCore(MyInfo()->aimId(), chatData);
                        Logic::getContactListModel()->clearChecked();
                        Ui::MainPage::instance()->clearSearchMembers();
                    }
                }
                else
                {
                    Logic::getContactListModel()->clearChecked();
                    Ui::MainPage::instance()->clearSearchMembers();
                }
            }
        }
    }

    bool callChatNameEditor(QWidget* _parent, GroupChatOperations::ChatData &chatData, Out std::shared_ptr<GroupChatSettings> &groupChatSettings)
    {
        const auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
        chatData.name = MyInfo()->friendlyName();
        if (chatData.name.isEmpty())
            chatData.name = MyInfo()->aimId();
        for (int i = 0, contactsSize = std::min((int)selectedContacts.size(), 2); i < contactsSize; ++i)
        {
            chatData.name += ql1s(", ") % selectedContacts[i].Get()->GetDisplayName();
        }
        auto chat_name = chatData.name;
        {
            groupChatSettings = std::make_shared<GroupChatSettings>(_parent, QT_TRANSLATE_NOOP("popup_window","DONE"), QT_TRANSLATE_NOOP("popup_window", "Chat settings"), chatData);
            auto result = groupChatSettings->show();
            if (chat_name != chatData.name)
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_rename);
            chatData.name = chatData.name.isEmpty() ? chat_name : chatData.name;
            return result;
        }
    }

    void postCreateChatInfoToCore(const QString &_aimId, const GroupChatOperations::ChatData &chatData, const QString& avatarId)
    {
        if (chatData.publicChat)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_public);
        if (chatData.approvedJoin)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_approval);
        if (chatData.readOnly)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_readonly);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        const auto aimIdAsUtf8 = _aimId.toUtf8();
        collection.set_value_as_string("aimid", aimIdAsUtf8.data(), aimIdAsUtf8.size());
        const auto nameAsUtf8 = chatData.name.toUtf8();
        collection.set_value_as_string("name", nameAsUtf8.data(), nameAsUtf8.size());
        const auto avatarIdAsUtf8 = avatarId.toUtf8();
        collection.set_value_as_string("avatar", avatarIdAsUtf8.data(), avatarIdAsUtf8.size());
        {
            const auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
            core::ifptr<core::iarray> members_array(collection->create_array());
            members_array->reserve(static_cast<int>(selectedContacts.size()));
            for (const auto& contact : selectedContacts)
            {
                const auto member = contact.get_aimid().toUtf8();
                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_string(member.data(), member.size());
                members_array->push_back(val.get());
            }
            collection.set_value_as_array("members", members_array.get());
        }
        collection.set_value_as_bool("public", chatData.publicChat);
        collection.set_value_as_bool("approved", chatData.approvedJoin);
        collection.set_value_as_bool("link", chatData.joiningByLink);
        collection.set_value_as_bool("ro", chatData.readOnly);
        collection.set_value_as_bool("age", chatData.ageGate);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/create"), collection.get());
    }

    void postAddChatMembersFromCLModelToCore(const QString& _aimId)
    {
        const auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
        Logic::getContactListModel()->clearChecked();

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        QStringList chat_members;
        chat_members.reserve(static_cast<int>(selectedContacts.size()));
        for (const auto& contact : selectedContacts)
            chat_members.push_back(contact.get_aimid());

        const auto aimIdAsUtf8 = _aimId.toUtf8();
        collection.set_value_as_string("aimid", aimIdAsUtf8.data(), aimIdAsUtf8.size());
        collection.set_value_as_string("m_chat_members_to_add", chat_members.join(ql1c(';')).toStdString());
        Ui::GetDispatcher()->post_message_to_core(qsl("add_members"), collection.get());
    }

    void deleteMemberDialog(Logic::ChatMembersModel* _model, const QString& current, int _regim, QWidget* _parent)
    {
        QString text;

        const bool isVideoConference = _regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE;

        if (_regim == Logic::MembersWidgetRegim::MEMBERS_LIST || _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from this chat?");
        else if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from ignore list?");
        else if (isVideoConference)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from this call?");

        auto left_button_text = QT_TRANSLATE_NOOP("popup_window", "CANCEL");
        auto right_button_text = QT_TRANSLATE_NOOP("popup_window", "DELETE");

        auto member = _model->getMemberItem(current);
        if (!member)
        {
            assert(!"member not found in model, probably it need to refresh");
            return;
        }
        auto user_name = member->NickName_;
        auto label = user_name.isEmpty() ? member->AimId_ : user_name;

        if (Utils::GetConfirmationWithTwoButtons(left_button_text, right_button_text, text, label, _parent,
            isVideoConference ? _parent->parentWidget() : Utils::InterConnector::instance().getMainWindow()))
        {
            if (_regim == Logic::MembersWidgetRegim::MEMBERS_LIST || _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS)
            {
                const auto aimIdAsUtf8 = _model->getChatAimId().toUtf8();
                ::Ui::gui_coll_helper collection(::Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_string("aimid", aimIdAsUtf8.data(), aimIdAsUtf8.size());
                collection.set_value_as_string("m_chat_members_to_remove", current.toStdString());
                Ui::GetDispatcher()->post_message_to_core("remove_members", collection.get());
            }
            else if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
            {
                Logic::getContactListModel()->ignoreContact(current, false);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignorelist_remove);
                // NOTE : when delete from ignore list, we don't need add contact to CL
            }
            else if (isVideoConference)
            {
                Ui::GetDispatcher()->getVoipController().setDecline(current.toStdString().c_str(), false);
            }
        }
    }



    void forwardMessage(const QVector<Data::Quote>& quotes, bool fromMenu)
    {
        SelectContactsWidget shareDialog(QT_TRANSLATE_NOOP("popup_window", "Forward"), Ui::MainPage::instance());

        emit Utils::InterConnector::instance().searchEnd();
        const auto action = shareDialog.show();
        if (action == QDialog::Accepted)
        {
            const auto contact = shareDialog.getSelectedContact();

            if (!contact.isEmpty())
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set<QString>("contact", contact);
                collection.set_value_as_string("message", std::string());
                if (!quotes.isEmpty())
                {
                    core::ifptr<core::iarray> quotesArray(collection->create_array());
                    quotesArray->reserve(quotes.size());

                    const auto senderChat = Logic::getContactListModel()->getContactItem(quotes.first().chatId_);
                    QString chatName(senderChat? senderChat->Get()->GetDisplayName(): QString());
                    QString chatStamp;
                    if (senderChat && senderChat->is_chat())
                        chatStamp = senderChat->get_stamp();

                    for (auto quote : quotes)
                    {
                        core::ifptr<core::icollection> quoteCollection(collection->create_collection());
                        quote.isForward_ = true;

                        // fix before new forward and quote logic
                        quote.chatName_ = chatName;
                        quote.chatStamp_ = chatStamp;
                        if (quote.chatId_ != quotes.first().chatId_)
                            quote.chatId_ = quotes.first().chatId_;

                        // uncomment when new forward logic arrives
                        //const auto senderChat = Logic::getContactListModel()->getContactItem(quote.chatId_);
                        //if (senderChat)
                        //{
                        //    quote.chatName_ = senderChat->Get()->GetDisplayName();
                        //    if (senderChat->is_chat())
                        //        quote.chatStamp_ = senderChat->get_stamp();
                        //}

                        quote.serialize(quoteCollection.get());
                        core::coll_helper coll(collection->create_collection(), true);
                        core::ifptr<core::ivalue> val(collection->create_value());
                        val->set_as_collection(quoteCollection.get());
                        quotesArray->push_back(val.get());
                    }
                    collection.set_value_as_array("quotes", quotesArray.get());

                    Data::MentionMap mentions;
                    for (const auto& q : quotes)
                        mentions.insert(q.mentions_.begin(), q.mentions_.end());

                    if (!mentions.empty())
                    {
                        core::ifptr<core::iarray> mentArray(collection->create_array());
                        mentArray->reserve(mentions.size());
                        for (const auto& m : mentions)
                        {
                            core::ifptr<core::icollection> mentCollection(collection->create_collection());
                            Ui::gui_coll_helper coll(mentCollection.get(), false);
                            coll.set_value_as_qstring("aimid", m.first);
                            coll.set_value_as_qstring("friendly", m.second);

                            core::ifptr<core::ivalue> val(collection->create_value());
                            val->set_as_collection(mentCollection.get());
                            mentArray->push_back(val.get());
                        }
                        collection.set_value_as_array("mentions", mentArray.get());
                    }
                }

                Ui::GetDispatcher()->post_message_to_core(qsl("send_message"), collection.get());

                core::stats::event_props_type props;
                props.push_back(std::make_pair("Forward_MessagesCount", std::to_string(quotes.size())));
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::forward_messagescount, props);

                Ui::GetDispatcher()->post_stats_to_core(fromMenu ? core::stats::stats_event_names::forward_send_frommenu : core::stats::stats_event_names::forward_send_frombutton);

                emit Utils::InterConnector::instance().addPageToDialogHistory(Logic::getContactListModel()->selectedContact());
                Logic::getContactListModel()->setCurrent(contact, -1, true);

                Utils::InterConnector::instance().onSendMessage(contact);
            }
        }
    }

    void forwardMessage(const QString& _message, bool fromMenu)
    {
        SelectContactsWidget shareDialog(QT_TRANSLATE_NOOP("popup_window", "Forward"), Ui::MainPage::instance());

        emit Utils::InterConnector::instance().searchEnd();
        const auto action = shareDialog.show();
        if (action == QDialog::Accepted)
        {
            const auto contact = shareDialog.getSelectedContact();

            if (!contact.isEmpty())
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

                collection.set<QString>("contact", contact);
                const auto messageAsoUtf8 = _message.toUtf8();
                collection.set_value_as_string("message", messageAsoUtf8.data(), messageAsoUtf8.size());

                Ui::GetDispatcher()->post_message_to_core(qsl("send_message"), collection.get());

                core::stats::event_props_type props;
                props.push_back(std::make_pair("Forward_MessagesCount", "1"));
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::forward_messagescount, props);
                Ui::GetDispatcher()->post_stats_to_core(fromMenu ? core::stats::stats_event_names::forward_send_frommenu : core::stats::stats_event_names::forward_send_frombutton);

                emit Utils::InterConnector::instance().addPageToDialogHistory(Logic::getContactListModel()->selectedContact());
                Logic::getContactListModel()->setCurrent(contact, -1, true);

                Utils::InterConnector::instance().onSendMessage(contact);
            }
        }
    }

}
