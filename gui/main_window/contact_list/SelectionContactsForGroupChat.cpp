#include "stdafx.h"
#include "SelectionContactsForGroupChat.h"

#include "AbstractSearchModel.h"
#include "ChatMembersModel.h"
#include "ContactList.h"
#include "ContactListWidget.h"
#include "ContactListItemRenderer.h"
#include "ContactListModel.h"
#include "SearchWidget.h"
#include "../GroupChatOperations.h"
#include "../MainWindow.h"
#include "../../core_dispatcher.h"
#include "../../controls/GeneralDialog.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

namespace Ui
{
    const double heightPartOfMainWindowForFullView = 0.6;
    const int dialogWidth = 360;
    const int search_padding_ver = 8;
    const int search_padding_hor = 4;

    bool SelectContactsWidget::forwardConfirmed(const QString& aimId)
    {
        QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to forward messages to <USER>?");
        bool replaced = false;
        if (auto contactItemWrapper = Logic::getContactListModel()->getContactItem(aimId))
        {
            if (auto contactItem = contactItemWrapper->Get())
            {
                text.replace(ql1s("<USER>"), contactItem->GetDisplayName());
                replaced = true;
            }
        }
        if (!replaced)
            text.replace(ql1s("<USER>"), contactList_->getSelectedAimid());

        auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Confirmation"),
            nullptr);

        return confirmed;
    }

    void SelectContactsWidget::enterPressed()
    {
        assert(contactList_);

        if (isShareLinkMode() || isShareTextMode())
        {
            selectedContact_ = contactList_->getSelectedAimid();
            bool confirmed = true;
            if (!selectedContact_.isEmpty())
            {
                mainDialog_->hide();
                confirmed = forwardConfirmed(selectedContact_);
            }
            if (confirmed)
            {
                if (isShareLinkMode())
                {
                    mainDialog_->accept();
                }
                else if (isShareTextMode())
                {
                    mainDialog_->accept();
                }
            }
            return;
        }
        else if (regim_ == Logic::CONTACT_LIST_POPUP)
        {
            selectedContact_ = contactList_->getSelectedAimid();
            if (!selectedContact_.isEmpty())
            {
                emit Logic::getContactListModel()->select(selectedContact_, -1);
                mainDialog_->accept();
            }
        }

        contactList_->searchResult();
    }

    void SelectContactsWidget::itemClicked(const QString& _current)
    {
        // Restrict max selected elements.
        if (regim_ == Logic::VIDEO_CONFERENCE && maximumSelectedCount_ >= 0)
        {
			bool isForCheck = !Logic::getContactListModel()->getIsChecked(_current);

            int selectedItemCount = Logic::getContactListModel()->GetCheckedContacts().size();
            if (isForCheck && selectedItemCount >= maximumSelectedCount_)
            {
                // Disable selection.
                return;
            }
        }

        selectedContact_ = _current;

        if (isShareLinkMode() || isShareTextMode())
        {
            bool confirmed = true;
            if (!selectedContact_.isEmpty())
            {
                mainDialog_->hide();
                confirmed = forwardConfirmed(selectedContact_);
            }
            if (confirmed)
            {
                if (isShareLinkMode())
                {
                    mainDialog_->accept();
                }
                else if (isShareTextMode())
                {
                    mainDialog_->accept();
                }
            }
            return;
        }

        if (Logic::is_members_regim(regim_))
        {
            auto globalCursorPos = mainDialog_->mapFromGlobal(QCursor::pos());
            auto removeFrame = ::ContactList::GetContactListParams().removeContactFrame();

            if (removeFrame.left() <= globalCursorPos.x() && globalCursorPos.x() <= removeFrame.right())
            {
                deleteMemberDialog(chatMembersModel_, _current, regim_, this);
                return;
            }

            if (regim_ != Logic::MembersWidgetRegim::IGNORE_LIST)
            {
                emit ::Utils::InterConnector::instance().profileSettingsShow(_current);
                if (platform::is_apple())
                {
                    mainDialog_->close();
                }
                else
                {
                    show();
                }
            }

            return;
        }

        // Disable delete for video conference list.
        if (chatMembersModel_ && !Logic::is_video_conference_regim(regim_) && regim_ != Logic::CONTACT_LIST_POPUP)
        {
            if (chatMembersModel_->isContactInChat(_current))
            {
                return;
            }
        }

        Logic::getContactListModel()->setChecked(_current, !Logic::getContactListModel()->getIsChecked(_current));
        mainDialog_->setButtonActive(!Logic::getContactListModel()->GetCheckedContacts().empty());
        contactList_->update();
    }

    SelectContactsWidget::SelectContactsWidget(const QString& _labelText, QWidget* _parent)
        : QDialog(_parent)
        , regim_(Logic::MembersWidgetRegim::SHARE_TEXT)
        , chatMembersModel_(nullptr)
        , isShortView_(false)
        , mainWidget_(new QWidget(this))
        , sortCL_(true)
        , handleKeyPressEvents_(true)
        , maximumSelectedCount_(-1)
		, searchModel_(nullptr)
    {
        init(_labelText);
    }

    SelectContactsWidget::SelectContactsWidget(Logic::ChatMembersModel* _chatMembersModel, int _regim, const QString& _labelText,
        const QString& _buttonText, QWidget* _parent, bool _handleKeyPressEvents/* = true*/,
		Logic::AbstractSearchModel* searchModel /*= nullptr*/)
        : QDialog(_parent)
        , regim_(_regim)
        , chatMembersModel_(_chatMembersModel)
        , isShortView_(false)
        , mainWidget_(new QWidget(this))
        , handleKeyPressEvents_(_handleKeyPressEvents)
        , maximumSelectedCount_(-1)
        , searchModel_(searchModel)
    {
        init(_labelText, _buttonText);
    }

    void SelectContactsWidget::init(const QString& _labelText, const QString& _buttonText)
    {
        globalLayout_ = Utils::emptyVLayout(mainWidget_);
        mainWidget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        mainWidget_->setFixedWidth(Utils::scale_value(dialogWidth));

        searchWidget_ = new SearchWidget(nullptr, Utils::scale_value(search_padding_hor), Utils::scale_value(search_padding_ver));
        Testing::setAccessibleName(searchWidget_, qsl("CreateGroupChat"));
        globalLayout_->addWidget(searchWidget_);

        contactList_ = new ContactListWidget(this, (Logic::MembersWidgetRegim)regim_, chatMembersModel_, searchModel_);
        contactList_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        searchModel_ = contactList_->getSearchModel();

        globalLayout_->addWidget(contactList_);

        auto contactsLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
        globalLayout_->addSpacerItem(contactsLayoutSpacer);

        // TODO : use SetView here
        mainDialog_ = std::make_unique<Ui::GeneralDialog>(mainWidget_,
            (isVideoConference() ? parentWidget() : Utils::InterConnector::instance().getMainWindow()), handleKeyPressEvents_);

        mainDialog_->addHead();
        mainDialog_->addLabel(_labelText);

		const auto is_show_button = !isShareTextMode() && !Logic::is_members_regim(regim_) && !Logic::is_video_conference_regim(regim_);
        if (is_show_button && regim_ != Logic::MembersWidgetRegim::CONTACT_LIST_POPUP)
        {
            mainDialog_->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), _buttonText, isShareLinkMode());
        }

        Testing::setAccessibleName(mainDialog_.get(), qsl("SelectContactsWidget"));

        connect(contactList_, &ContactListWidget::searchEnd, searchWidget_, &SearchWidget::searchCompleted, Qt::QueuedConnection);
        connect(contactList_, &ContactListWidget::itemSelected, this, &SelectContactsWidget::itemClicked, Qt::QueuedConnection);

        connect(searchWidget_, &SearchWidget::enterPressed, this, &SelectContactsWidget::enterPressed, Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::escapePressed, this, &SelectContactsWidget::escapePressed, Qt::QueuedConnection);
        contactList_->connectSearchWidget(searchWidget_);

        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP)
            connect(contactList_, &ContactListWidget::itemSelected, this, &QDialog::reject, Qt::QueuedConnection);
    }

    const QString& SelectContactsWidget::getSelectedContact() const
    {
        return selectedContact_;
    }

    SelectContactsWidget::~SelectContactsWidget()
    {
    }

    bool SelectContactsWidget::isCheckboxesVisible() const
    {
        if (Logic::is_members_regim(regim_))
        {
            return false;
        }

        if (isShareLinkMode() || isShareTextMode())
        {
            return false;
        }

        return true;
    }

    bool SelectContactsWidget::isShareLinkMode() const
    {
        return (regim_ == Logic::MembersWidgetRegim::SHARE_LINK);
    }

    bool SelectContactsWidget::isShareTextMode() const
    {
        return (regim_ == Logic::MembersWidgetRegim::SHARE_TEXT);
    }

    bool SelectContactsWidget::isVideoConference() const
    {
        return (regim_ == Logic::MembersWidgetRegim::VIDEO_CONFERENCE);
    }

    void SelectContactsWidget::searchEnd()
    {
        emit contactList_->searchEnd();
        // contactList_->setSearchMode(false);
    }

    void SelectContactsWidget::escapePressed()
    {
        mainDialog_->close();
    }

    QRect SelectContactsWidget::CalcSizes() const
    {
        contactList_->setWidthForDelegate(Utils::scale_value(dialogWidth));

        auto newHeight = ::ContactList::ItemLength(false, heightPartOfMainWindowForFullView, 0,
            (isVideoConference() ? parentWidget() : Utils::InterConnector::instance().getMainWindow()));
        auto extraHeight = searchWidget_->sizeHint().height();
        if (!Logic::is_members_regim(regim_))
            extraHeight += Utils::scale_value(42);

        if (Logic::is_members_regim(regim_)
            && isShortView_
            && chatMembersModel_->getMembersCount() >= Logic::InitMembersLimit)
        {
            extraHeight += Utils::scale_value(52);
        }

        auto itemHeight = ::ContactList::GetContactListParams().itemHeight();
        int count = (newHeight - extraHeight) / itemHeight;

        int clHeight = (count + 0.5) * itemHeight;
        if (Logic::is_members_regim(regim_))
        {
            count = chatMembersModel_->rowCount();
            clHeight = std::min(clHeight, count * itemHeight + Utils::scale_value(1));

            extraHeight += itemHeight / 2;
            if (count == 0 && regim_ == Logic::MembersWidgetRegim::IGNORE_LIST)
                extraHeight += itemHeight + Utils::scale_value(5);
        }

        newHeight = extraHeight + clHeight;

        return QRect(0, 0, Utils::scale_value(dialogWidth), newHeight);
    }

    bool SelectContactsWidget::show(int _x, int _y)
    {
        x_ = _x;
        y_ = _y;

        Logic::getContactListModel()->setIsWithCheckedBox(!isShareLinkMode() && !isShareTextMode());

        mainDialog_->setButtonActive(isShareLinkMode() || isShareTextMode() || !Logic::getContactListModel()->GetCheckedContacts().empty());

        auto newRect = CalcSizes();
        mainWidget_->setFixedSize(newRect.width(), newRect.height());

        searchWidget_->setFocus();

        auto result = mainDialog_->showInPosition(x_, y_);
        if (result)
        {
            if (!isShareTextMode()
                && !isShareLinkMode()
                && Logic::getContactListModel()->GetCheckedContacts().empty())
            {
                result = false;
            }
        }
        else
        {
            Logic::getContactListModel()->clearChecked();
        }

        Logic::getContactListModel()->setIsWithCheckedBox(false);
        searchWidget_->clearInput();
        return result;
    }

    bool SelectContactsWidget::show()
    {
        return show(defaultInvalidCoord, defaultInvalidCoord);
    }

    void SelectContactsWidget::setView(bool _isShortView)
    {
        chatMembersModel_->isShortView_ = _isShortView;
        searchWidget_->setVisible(!_isShortView);
        isShortView_ = _isShortView;
    }

    void SelectContactsWidget::onViewAllMembers()
    {
        setView(false);
        chatMembersModel_->loadAllMembers();
        x_ = y_ = defaultInvalidCoord;
        UpdateView();
    }

    void SelectContactsWidget::UpdateView()
    {
        auto newRect = CalcSizes();
        mainWidget_->setFixedSize(newRect.width(), newRect.height());
    }

    void SelectContactsWidget::UpdateViewForIgnoreList(bool _isEmptyIgnoreList)
    {
        contactList_->setEmptyIgnoreLabelVisible(_isEmptyIgnoreList);
        UpdateView();
    }

    void SelectContactsWidget::UpdateMembers()
    {
        mainDialog_->update();
    }

    void SelectContactsWidget::setMaximumSelectedCount(int number)
    {
        maximumSelectedCount_ = number;
    }

    void SelectContactsWidget::UpdateContactList()
    {
        contactList_->update();
    }

    void SelectContactsWidget::reject()
    {
        if (mainDialog_)
        {
            mainDialog_->reject();
        }
    }
}
