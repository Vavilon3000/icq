#include "stdafx.h"
#include "ContactListModel.h"

#include "contact_profile.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "../MainWindow.h"
#include "../history_control/HistoryControlPage.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../my_info.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace
{
    const std::chrono::milliseconds sort_timer_timeout = std::chrono::seconds(30);
}

namespace Logic
{
    std::unique_ptr<ContactListModel> g_contact_list_model;

    ContactListModel::ContactListModel(QObject* _parent)
        : CustomAbstractListModel(_parent)
        , ref_(new bool(false))
        , gotPageCallback_(nullptr)
        , sortNeeded_(false)
        , sortTimer_(new QTimer(this))
        , scrollPosition_(0)
        , minVisibleIndex_(0)
        , maxVisibleIndex_(0)
        , searchRequested_(false)
        , isSearch_(false)
        , isWithCheckedBox_(false)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::contactList,     this, &ContactListModel::contactList);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::presense,        this, &ContactListModel::presence);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::outgoingMsgCount,this, &ContactListModel::outgoingMsgCount);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::contactRemoved,  this, &ContactListModel::contactRemoved);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo,        this, &ContactListModel::chatInfo);

        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged,   this, &ContactListModel::avatarLoaded);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clSortChanged, this, &ContactListModel::forceSort);

        connect(sortTimer_, &QTimer::timeout, this, &ContactListModel::sort);
        sortTimer_->setTimerType(Qt::CoarseTimer);
        sortTimer_->start(sort_timer_timeout.count());
    }

    int ContactListModel::rowCount(const QModelIndex &) const
    {
        return visible_indexes_.size();
    }

    int ContactListModel::getAbsIndexByVisibleIndex(const int& _visibleIndex) const
    {
        if (_visibleIndex >= 0 && _visibleIndex < visible_indexes_.size())
            return visible_indexes_[_visibleIndex];
        return 0;
    }

    QModelIndex ContactListModel::contactIndex(const QString& _aimId) const
    {
        const auto orderedIndex = getOrderIndexByAimid(_aimId);
        const auto visIter = std::find(visible_indexes_.cbegin(), visible_indexes_.cend(), orderedIndex);
        if (visIter != visible_indexes_.cend())
            return index(*visIter);
        return index(0);
    }

    QVariant ContactListModel::data(const QModelIndex& _i, int _r) const
    {
        int cur = _i.row();

        if (!_i.isValid() || (unsigned)cur >= contacts_.size())
        {
            return QVariant();
        }

        if (_r == Qt::DisplayRole)
        {
            if (maxVisibleIndex_ == 0 && minVisibleIndex_ == 0)
                maxVisibleIndex_ = minVisibleIndex_ = cur;

            if (cur < minVisibleIndex_)
                minVisibleIndex_ = cur;
            if (cur > maxVisibleIndex_)
                maxVisibleIndex_ = cur;
        }

        const auto ndx = getAbsIndexByVisibleIndex(cur);
        const auto cont = contacts_[ndx].Get();

        if (Testing::isAccessibleRole(_r))
            return cont->AimId_;

        return QVariant::fromValue(cont);
    }

    QString ContactListModel::contactToTryOnTheme() const
    {
        if (!currentAimId_.isEmpty())
            return currentAimId_;

        QString recentsFirstContact = Logic::getRecentsModel()->firstContact();
        if (!recentsFirstContact.isEmpty())
            return recentsFirstContact;

        if (!contacts_.empty())
            return contacts_[getIndexByOrderedIndex(0)].get_aimid();

        return QString();
    }

    Qt::ItemFlags ContactListModel::flags(const QModelIndex& _i) const
    {
        if (!_i.isValid())
            return Qt::ItemIsEnabled;

        unsigned flags = QAbstractItemModel::flags(_i) | Qt::ItemIsEnabled;

        const auto ndx = getAbsIndexByVisibleIndex(_i.row());
        const auto& cont = contacts_[ndx];

        if (_i.row() == 0 || cont.is_group())
            flags |= ~Qt::ItemIsSelectable;
        flags |= ~Qt::ItemIsEditable;

        return (Qt::ItemFlags)flags;
    }

    bool ContactListModel::isVisibleItem(const ContactItem& _item) const
    {
        if (!isWithCheckedBox_)
        {
            return true;
        }

        if (_item.is_chat())
        {
            return false;
        }

        return (Ui::MyInfo()->aimId() != _item.get_aimid());
    }

    void ContactListModel::setFocus()
    {
    }

    int ContactListModel::addItem(Data::ContactPtr _contact, const bool _updatePlaceholder)
    {
        auto item = getContactItem(_contact->AimId_);
        if (item != nullptr)
        {
            item->Get()->ApplyBuddy(_contact);
            setContactVisible(_contact->AimId_, true);
            Logic::GetAvatarStorage()->UpdateDefaultAvatarIfNeed(_contact->AimId_);

            const auto ndx = contactIndex(_contact->AimId_).row();
            emitChanged(ndx, ndx);
        }
        else
        {
            contacts_.emplace_back(_contact);

            const auto newNdx = (int)contacts_.size() - 1;
            sorted_index_cl_.emplace_back(newNdx);
            visible_indexes_.emplace_back(newNdx);
            sorted_index_recents_.emplace_back(newNdx);
            indexes_.insert(_contact->AimId_, newNdx);

            emitChanged(newNdx, newNdx);
        }

        sortNeeded_ = true;

        if (_updatePlaceholder)
            updatePlaceholders();

        return (int)contacts_.size();
    }

    void ContactListModel::rebuildIndex()
    {
        int i = 0;

        indexes_.clear();
        for (const auto &order_index : sorted_index_cl_)
            indexes_.insert(contacts_[order_index].get_aimid(), i++);

        rebuildVisibleIndex();
    }

    void ContactListModel::rebuildVisibleIndex()
    {
        const auto groups_enabled = Ui::get_gui_settings()->get_value<bool>(settings_cl_groups_enabled, false);

        visible_indexes_.clear();
        for (const auto &order_index : sorted_index_cl_)
        {
            const auto& contact = contacts_[order_index];

            if (contact.is_visible() && (groups_enabled || !contact.is_group()))
                visible_indexes_.emplace_back(order_index);
        }
    }

    void ContactListModel::contactList(std::shared_ptr<Data::ContactList> _cl, const QString& _type)
    {
        const int size = (int)contacts_.size();
        beginInsertRows(QModelIndex(), size, _cl->size() + size);

        bool needSyncSort = contacts_.empty();

        const bool isDeletedType = _type == ql1s("deleted");

        for (auto it = _cl->keyBegin(), end = _cl->keyEnd(); it != end; ++it)
        {
            const auto& iter = *it;
            if (!isDeletedType)
            {
                if (iter->UserType_ != ql1s("sms"))
                {
                    addItem(iter, false);
                    if (iter->IsLiveChat_)
                        emit liveChatJoined(iter->AimId_);
                    emit contactChanged(iter->AimId_);
                }
            }
            else
            {
                innerRemoveContact(iter->AimId_);
            }
        }

        std::vector<int> groupIds;
        for (const auto& iter : *_cl)
        {
            if (std::find(groupIds.begin(), groupIds.end(), iter->Id_) != groupIds.end())
                continue;

            if (!isDeletedType)
            {
                groupIds.push_back(iter->Id_);
                auto group = std::make_shared<Data::Group>();
                group->ApplyBuddy(iter);
                addItem(group, false);
            }
            else if (iter->Removed_)
            {
                QVector<QString> toRemove;
                for (const auto& contact: contacts_)
                {
                    if (contact.Get()->GroupId_ == iter->Id_)
                        toRemove.push_back(contact.get_aimid());
                }
                removeContactsFromModel(toRemove);
            }
        }
        endInsertRows();

        rebuildIndex();
        updatePlaceholders();
        sortNeeded_ = true;

        if (needSyncSort)
            sort();
    }

    void ContactListModel::updatePlaceholders()
    {
        if (contacts_.empty())
        {
            emit Utils::InterConnector::instance().showNoContactsYet();
            emit Utils::InterConnector::instance().showNoRecentsYet();
        }
        else if (visible_indexes_.empty())
            emit Utils::InterConnector::instance().hideNoContactsYet();
    }

    int ContactListModel::innerRemoveContact(const QString& _aimId)
    {
        size_t idx = 0;
        for ( ; idx < contacts_.size(); ++idx)
        {
            if (contacts_[idx].Get()->AimId_ == _aimId)
                break;
        }

        auto cont = contacts_.begin() + idx;
        if (cont == contacts_.end())
            return contacts_.size();

        if (cont->is_live_chat())
            emit liveChatRemoved(_aimId);

        contacts_.erase(cont);

        updateIndexesListAfterRemoveContact(sorted_index_cl_, idx);
        updateIndexesListAfterRemoveContact(visible_indexes_, idx);
        updateIndexesListAfterRemoveContact(sorted_index_recents_, idx);

        emit contact_removed(_aimId);
        return idx;
    }

    void ContactListModel::updateIndexesListAfterRemoveContact(std::vector<int>& _list, int _index)
    {
        _list.erase(std::remove(_list.begin(), _list.end(), _index), _list.end());
        for (auto& idx : _list)
        {
            if (idx > _index)
            {
                idx -= 1;
            }
        }
    }

    void ContactListModel::contactRemoved(const QString& _contact)
    {
        innerRemoveContact(_contact);
        rebuildIndex();
        updatePlaceholders();
        sortNeeded_ = true;
    }

    void ContactListModel::outgoingMsgCount(const QString & _aimid, const int _count)
    {
        auto contact = getContactItem(_aimid);
        if (contact && contact->get_outgoing_msg_count() != _count)
        {
            contact->set_outgoing_msg_count(_count);
            sortNeeded_ = true;
        }
    }

    void ContactListModel::avatarLoaded(const QString& _aimId)
    {
        auto idx = getOrderIndexByAimid(_aimId);
        if (idx != -1)
        {
            emitChanged(idx, idx);
        }
    }

    void ContactListModel::presence(std::shared_ptr<Data::Buddy> _presence)
    {
        auto contact = getContactItem(_presence->AimId_);
        if (contact)
        {
            contact->Get()->ApplyBuddy(_presence);
            sortNeeded_ = true;

            pushChange(getOrderIndexByAimid(_presence->AimId_));
            emit contactChanged(_presence->AimId_);
        }
    }

    void ContactListModel::groupClicked(int _groupId)
    {
        for (auto& contact: contacts_)
        {
            if (contact.Get()->GroupId_ == _groupId && !contact.is_group())
            {
                contact.set_visible(!contact.is_visible());

                const auto index = contactIndex(contact.get_aimid()).row();
                emitChanged(index, index);
            }
        }
        rebuildVisibleIndex();
    }

    void ContactListModel::scrolled(int _value)
    {
        minVisibleIndex_ = 0;
        maxVisibleIndex_ = 0;
        scrollPosition_ = _value;
    }

    void ContactListModel::pushChange(int _i)
    {
        updatedItems_.push_back(_i);
        int scrollPos = scrollPosition_;
        QTimer::singleShot(2000, this, [this, scrollPos](){ if (scrollPosition_ == scrollPos) processChanges(); });
    }

    void ContactListModel::processChanges()
    {
        if (updatedItems_.empty())
            return;

        for (auto iter : updatedItems_)
        {
            if (iter >= minVisibleIndex_ && iter <= maxVisibleIndex_)
                emitChanged(iter, iter);
        }

        updatedItems_.clear();
    }

    void ContactListModel::setCurrent(const QString& _aimId, qint64 id, bool _sel, std::function<void(Ui::HistoryControlPage*)> _gotPageCallback, qint64 quote_id)
    {
        if (_gotPageCallback)
        {
            auto page = Utils::InterConnector::instance().getMainWindow()->getHistoryPage(_aimId);
            if (page)
            {
                _gotPageCallback(page);
            }
            else
            {
                gotPageCallback_ = _gotPageCallback;
            }
        }

        currentAimId_ = _aimId;
        emit selectedContactChanged(currentAimId_);

        if (!currentAimId_.isEmpty() && _sel)
            emit select(currentAimId_, id, quote_id);

        auto recentState = Logic::getRecentsModel()->getDlgState(_aimId, false);
        auto unknownState = Logic::getUnknownsModel()->getDlgState(_aimId, false);
        if (recentState.AimId_ != _aimId && unknownState.AimId_ == _aimId)
            emit Utils::InterConnector::instance().unknownsGoSeeThem();
        else if (!_aimId.isEmpty())
            emit Utils::InterConnector::instance().unknownsGoBack();
    }

    void ContactListModel::setCurrentCallbackHappened(Ui::HistoryControlPage* _page)
    {
        if (gotPageCallback_)
        {
            gotPageCallback_(_page);
            gotPageCallback_ = nullptr;
        }
    }

    void ContactListModel::sort()
    {
        if (!sortNeeded_)
            return;

        updateSortedIndexesList(sorted_index_cl_, getLessFuncCL(QDateTime::currentDateTime()));
        rebuildIndex();
        sortNeeded_ = false;

        emitChanged(minVisibleIndex_, maxVisibleIndex_);
    }

    void ContactListModel::forceSort()
    {
        updateSortedIndexesList(sorted_index_cl_, getLessFuncCL(QDateTime::currentDateTime()));
        rebuildIndex();
        emitChanged(minVisibleIndex_, maxVisibleIndex_);
    }

    ContactListSorting::contact_sort_pred ContactListModel::getLessFuncCL(const QDateTime& current) const
    {
        ContactListSorting::contact_sort_pred less = ContactListSorting::ItemLessThanNoGroups(current);

        if (Ui::get_gui_settings()->get_value<bool>(settings_cl_groups_enabled, false))
            less = ContactListSorting::ItemLessThan();

        return less;
    }

    void ContactListModel::updateSortedIndexesList(std::vector<int>& _list, ContactListSorting::contact_sort_pred _less)
    {
        _list.resize(contacts_.size());
        std::iota(_list.begin(), _list.end(), 0);

        auto show_popular_contacts = Ui::get_gui_settings()->get_value<bool>(settings_show_popular_contacts, true);
        if (show_popular_contacts)
        {
            std::sort(_list.begin(), _list.end(), [this](const int& a, const int& b)
            {
                const auto& fc = contacts_[a];
                const auto& sc = contacts_[b];

                if (fc.get_outgoing_msg_count() == sc.get_outgoing_msg_count())
                {
                    const auto fdlg = Logic::getRecentsModel()->getDlgState(fc.get_aimid());
                    const auto sdlg = Logic::getRecentsModel()->getDlgState(sc.get_aimid());

                    if (fdlg.Time_ != -1 && sdlg.Time_ != -1)
                        return fdlg.Time_ > sdlg.Time_;

                    if (fdlg.Time_ == -1)
                        return false;

                    if (sdlg.Time_ == -1)
                        return true;
                }
                return fc.get_outgoing_msg_count() > sc.get_outgoing_msg_count();
            });
        }

        auto it = _list.begin();
        if (show_popular_contacts)
        {
            while (it != _list.end() && (it - _list.begin() < ContactListSorting::maxTopContactsByOutgoing))
            {
                if (contacts_[*it].get_outgoing_msg_count() == 0)
                    break;
                ++it;
            }
        }

        std::sort(it, _list.end(), [_less, this] (const int& a, const int& b)
        {
            return _less(contacts_[a], contacts_[b]);
        });
    }

    bool ContactListModel::contains(const QString& _aimdId) const
    {
        return indexes_.find(_aimdId) != indexes_.end();
    }

    QString ContactListModel::selectedContact() const
    {
        return currentAimId_;
    }

    QString ContactListModel::selectedContactName() const
    {
        auto contact = getContactItem(currentAimId_);
        return contact == nullptr ? QString() : contact->Get()->GetDisplayName();
    }

    void ContactListModel::add(const QString& _aimId, const QString& _friendly)
    {
        auto addContact = std::make_shared<Data::Contact>();
        addContact->AimId_ = _aimId;
        addContact->State_ = QString();
        addContact->NotAuth_ = true;
        addContact->Friendly_ = _friendly;
        addContact->Is_chat_ = _aimId.contains(ql1s("@chat.agent"));
        addItem(addContact);
    }

    void ContactListModel::setContactVisible(const QString& _aimId, bool _visible)
    {
        auto item = getContactItem(_aimId);
        if (item && item->is_visible() != _visible)
        {
            item->set_visible(_visible);
            rebuildVisibleIndex();
            emitChanged(0, indexes_.size());
        }
    }

    ContactItem* ContactListModel::getContactItem(const QString& _aimId)
    {
        return const_cast<ContactItem*>(static_cast<const ContactListModel*>(this)->getContactItem(_aimId));
    }

    const ContactItem* ContactListModel::getContactItem(const QString& _aimId) const
    {
        auto idx = getOrderIndexByAimid(_aimId);
        if (idx == -1)
            return nullptr;

        if (idx >= (int) contacts_.size())
            return nullptr;

        return &contacts_[getIndexByOrderedIndex(idx)];
	}

	QString ContactListModel::getDisplayName(const QString& _aimId) const
	{
		auto ci = getContactItem(_aimId);
		if (!ci)
		{
			return _aimId;
		}

		return ci->Get()->GetDisplayName();
	}

	bool ContactListModel::isChat(const QString& _aimId) const
	{
		auto ci = getContactItem(_aimId);
		if (!ci)
		{
			return false;
		}

		return ci->is_chat();
	}

	bool ContactListModel::isMuted(const QString& _aimId) const
	{
		auto ci = getContactItem(_aimId);
		if (!ci)
		{
			return false;
		}

		return ci->is_muted();
	}

    bool ContactListModel::isLiveChat(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return false;
        }

        return ci->is_live_chat();
    }

    bool ContactListModel::isOfficial(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return false;
        }

        return ci->is_official();
    }

    bool ContactListModel::isNotAuth(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return false;
        }

        return ci->is_not_auth();
    }

    QString ContactListModel::getState(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return QString();
        }
		return ci->Get()->GetState();
	}

    QString ContactListModel::getStatusString(const QString& _aimId) const
    {
        QString statusString;
        auto contact = getContactItem(_aimId);
        if (!contact)
            return statusString;
        if (isOfficial(_aimId))
        {
            statusString = QT_TRANSLATE_NOOP("chat_page", "Official account");
        }
        else if (isNotAuth(_aimId))
        {
            statusString = QT_TRANSLATE_NOOP("chat_page", "Not authorized");
        }
        else
        {
            QString state;
            QDateTime lastSeen = contact->Get()->LastSeen_;
            if (lastSeen.isValid())
            {
                state = getLastSeenString(_aimId);
            }
            else
            {
                state = contact->is_phone() ? contact->Get()->AimId_ :
                    (contact->Get()->StatusMsg_.isEmpty() ? contact->Get()->State_ : contact->Get()->StatusMsg_);
            }
            statusString = state;
        }
        return statusString;
    }

    QDateTime ContactListModel::getLastSeen(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return QDateTime::currentDateTime();
        }

        return ci->Get()->GetLastSeen();
    }

    QString ContactListModel::getLastSeenString(const QString& _aimId) const
    {
        QString slastSeen;
        const auto lastSeen = getLastSeen(_aimId);
        if (!lastSeen.isValid())
            return slastSeen;
        slastSeen = QT_TRANSLATE_NOOP("contact_list", "Seen ");
        QString slastSeenSuffix;
        const auto current = QDateTime::currentDateTime();
        const auto days = lastSeen.daysTo(current);
        if (days == 0)
            slastSeenSuffix += QT_TRANSLATE_NOOP("contact_list", "today");
        else if (days == 1)
            slastSeenSuffix += QT_TRANSLATE_NOOP("contact_list", "yesterday");
        else
            slastSeenSuffix += Utils::GetTranslator()->formatDate(lastSeen.date(), lastSeen.date().year() == current.date().year());
        if (lastSeen.date().year() == current.date().year())
            slastSeenSuffix += QT_TRANSLATE_NOOP("contact_list", " at ") % lastSeen.time().toString(Qt::SystemLocaleShortDate);
        if (!slastSeenSuffix.length())
            slastSeen = QString();
        else
            slastSeen += slastSeenSuffix;
        return slastSeen;
    }

    ContactListModel* getContactListModel()
    {
        if (!g_contact_list_model)
            g_contact_list_model = std::make_unique<Logic::ContactListModel>(nullptr);

        return g_contact_list_model.get();
    }

    void ResetContactListModel()
    {
        if (g_contact_list_model)
            g_contact_list_model.reset();
    }

    QString ContactListModel::getInputText(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return QString();
        }

        return ci->get_input_text();
    }

    void ContactListModel::setInputText(const QString& _aimId, const QString& _text)
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return;
        }

        ci->set_input_text(_text);
    }

    void ContactListModel::getContactProfile(const QString& _aimId, std::function<void(profile_ptr, int32_t error)> _callBack)
    {
        profile_ptr profile;

        if (!_aimId.isEmpty())
        {
            auto ci = getContactItem(_aimId);

            if (ci)
            {
                profile = ci->getContactProfile();
            }
        }

        if (profile)
        {
            emit profile_loaded(profile);

            _callBack(profile, 0);
        }

        if (!profile)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

            collection.set_value_as_qstring("aimid", _aimId);

            Ui::GetDispatcher()->post_message_to_core(qsl("contacts/profile/get"), collection.get(), this, [this, _callBack](core::icollection* _coll)
            {
                Ui::gui_coll_helper coll(_coll, false);

                const QString aimid = QString::fromUtf8(coll.get_value_as_string("aimid"));
                int32_t err = coll.get_value_as_int("error");

                if (err == 0)
                {
                    Ui::gui_coll_helper coll_profile(coll.get_value_as_collection("profile"), false);

                    profile_ptr profile(new contact_profile());

                    if (profile->unserialize(coll_profile))
                    {
                        if (!aimid.isEmpty())
                        {
                            ContactItem* ci = getContactItem(aimid);
                            if (ci)
                                ci->set_contact_profile(profile);
                        }

                        emit profile_loaded(profile);

                        _callBack(profile, 0);
                    }
                }
                else
                {
                    _callBack(nullptr, 0);
                }
            });
        }
    }

    void ContactListModel::addContactToCL(const QString& _aimId, std::function<void(bool)> _callBack)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_qstring("group", qsl("General"));
        collection.set_value_as_qstring("message", QT_TRANSLATE_NOOP("contact_list","Hello. Please add me to your contact list"));

        Ui::GetDispatcher()->post_message_to_core(qsl("contacts/add"), collection.get(), this, [this, _aimId, _callBack](core::icollection* _coll)
        {
            Logic::ContactItem* item = getContactItem(_aimId);
            if (item && item->is_not_auth())
            {
                item->reset_not_auth();
                emit contactChanged(_aimId);
            }

            Ui::gui_coll_helper coll(_coll, false);

            const QString contact = QString::fromUtf8(coll.get_value_as_string("contact"));

            int32_t err = coll.get_value_as_int("error");

            _callBack(err == 0);

            emit contact_added(contact, (err == 0));
            emit needSwitchToRecents();
        });
    }

    void ContactListModel::renameContact(const QString& _aimId, const QString& _friendly)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_qstring("friendly", _friendly);

        Ui::GetDispatcher()->post_message_to_core(qsl("contacts/rename"), collection.get());
    }

    void ContactListModel::renameChat(const QString& _aimId, const QString& _friendly)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("aimid", _aimId);
        collection.set_value_as_qstring("m_chat_name", _friendly);
        Ui::GetDispatcher()->post_message_to_core(qsl("modify_chat"), collection.get());
    }

    void ContactListModel::removeContactFromCL(const QString& _aimId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        Ui::GetDispatcher()->post_message_to_core(qsl("contacts/remove"), collection.get());
    }

    void ContactListModel::removeContactsFromModel(const QVector<QString>& _vcontacts)
    {
        for (const auto& _aimid : _vcontacts)
            innerRemoveContact(_aimid);
        rebuildIndex();
        updatePlaceholders();
        sortNeeded_ = true;
    }

    bool ContactListModel::blockAndSpamContact(const QString& _aimId, bool _withConfirmation /*= true*/)
    {
        bool confirm = false;
        if (_withConfirmation)
        {
            confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                QT_TRANSLATE_NOOP("popup_window", "YES"),
                QT_TRANSLATE_NOOP("popup_window", "Are you sure this contact is spam?"),
                getDisplayName(_aimId),
                nullptr);
        }

        if (confirm || !_withConfirmation)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", _aimId);
            Ui::GetDispatcher()->post_message_to_core(qsl("contacts/block"), collection.get());
        }
        return confirm;
    }

    void ContactListModel::ignoreContact(const QString& _aimId, bool _ignore)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_bool("ignore", _ignore);
        Ui::GetDispatcher()->post_message_to_core(qsl("contacts/ignore"), collection.get());

		if (_ignore)
		{
			emit ignore_contact(_aimId);
            Ui::GetDispatcher()->getVoipController().setDecline(_aimId.toStdString().c_str(), true);
		}
    }

    bool ContactListModel::ignoreContactWithConfirm(const QString& _aimId)
    {
        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to move contact to ignore list?"),
            getDisplayName(_aimId),
            nullptr);

        if (confirm)
        {
            ignoreContact(_aimId, true);
        }
        return confirm;
    }

    bool ContactListModel::isYouAdmin(const QString& _aimId)
    {
        auto cont = getContactItem(_aimId);
        if (cont)
        {
            const auto role = cont->get_chat_role();
            return role == ql1s("admin") || role == ql1s("moder");
        }

        return false;
    }

    QString ContactListModel::getYourRole(const QString& _aimId)
    {
        auto cont = getContactItem(_aimId);
        if (cont)
        {
            return cont->get_chat_role();
        }

        return QString();
    }

    void ContactListModel::setYourRole(const QString& _aimId, const QString& _role)
    {
        auto cont = getContactItem(_aimId);
        if (cont)
        {
            emit youRoleChanged(_aimId);
            return cont->set_chat_role(_role);
        }
    }

    void ContactListModel::getIgnoreList()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        Ui::GetDispatcher()->post_message_to_core(qsl("contacts/get_ignore"), collection.get());
    }

    void ContactListModel::emitChanged(const int _first, const int _last)
    {
        emit dataChanged(index(_first), index(_last));
    }

    std::vector<ContactItem> ContactListModel::GetCheckedContacts() const
    {
        std::vector<ContactItem> result;
        for (const auto& item : contacts_)
        {
            if (item.is_checked())
            {
                result.push_back(item);
            }
        }
        return result;
    }

    void ContactListModel::clearChecked()
    {
        for (auto& item : contacts_)
        {
            item.set_checked(false);
        }
    }

    void ContactListModel::setChecked(const QString& _aimid, bool _isChecked)
    {
        auto contact = getContactItem(_aimid);
        if (contact)
            contact->set_checked(_isChecked);
    }

    bool ContactListModel::getIsChecked(const QString& _aimId) const
    {
        auto contact = getContactItem(_aimId);
        if (contact)
            return contact->is_checked();
        return false;
    }

    bool ContactListModel::isWithCheckedBox()
    {
        return isWithCheckedBox_;
    }

    void ContactListModel::setIsWithCheckedBox(bool _isWithCheckedBox)
    {
        isWithCheckedBox_ = _isWithCheckedBox;
    }

    void ContactListModel::chatInfo(qint64, std::shared_ptr<Data::ChatInfo> _info)
    {
        auto contact = getContactItem(_info->AimId_);
        if (!contact)
            return;

        QString role = _info->YourRole_;
        if (_info->YouPending_)
            role = qsl("pending");
        else if (!_info->YouMember_ || role.isEmpty())
            role = qsl("notamember");

        if (contact->get_chat_role() != role)
            emit youRoleChanged(_info->AimId_);
        contact->set_chat_role(role);

        contact->set_stamp(_info->Stamp_);
    }

    void ContactListModel::joinLiveChat(const QString& _stamp, bool _silent)
    {
        if (_silent)
        {
            getContactProfile(Ui::MyInfo()->aimId(), [_stamp](profile_ptr profile, int32_t)
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("stamp", _stamp);
                if (profile)
                {
                    int age = Utils::calcAge(QDateTime::fromMSecsSinceEpoch((qint64)profile->get_birthdate() * 1000, Qt::LocalTime));
                    collection.set_value_as_int("age", age);
                }
                Ui::GetDispatcher()->post_message_to_core(qsl("livechat/join"), collection.get());
            });
        }
        else
        {
            emit needJoinLiveChat(_stamp);
        }
    }

    void ContactListModel::next()
    {
        int current = 0;
        if (!currentAimId_.isEmpty())
        {
            auto idx = getOrderIndexByAimid(currentAimId_);
            if (idx != -1)
                current = idx;
        }

        ++current;

        for (auto iter = indexes_.cbegin(), end = indexes_.cend(); iter != end; ++iter)
        {
            if (iter.value() == current)
            {
                setCurrent(iter.key(), -1, true);
                break;
            }
        }
    }

    void ContactListModel::prev()
    {
        int current = 0;
        if (!currentAimId_.isEmpty())
        {
            auto idx = getOrderIndexByAimid(currentAimId_);
            if (idx != -1)
                current = idx;
        }

        --current;

        for (auto iter = indexes_.cbegin(), end = indexes_.cend(); iter != end; ++iter)
        {
            if (iter.value() == current)
            {
                setCurrent(iter.key(), -1, true);
                break;
            }
        }
    }

    int ContactListModel::getIndexByOrderedIndex(int _index) const
    {
        return sorted_index_cl_[_index];
    }

    int ContactListModel::getOrderIndexByAimid(const QString& _aimId) const
    {
        assert(indexes_.size() == contacts_.size());

        if (_aimId.isEmpty())
            return -1;

        auto item = indexes_.find(_aimId);
        return item != indexes_.end() ? item.value() : -1;
    }
}
