#include "stdafx.h"

#include "MentionModel.h"
#include "SearchModelDLG.h"
#include "ContactListModel.h"
#include "../../core_dispatcher.h"
#include "../../my_info.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace Logic
{
    std::unique_ptr<MentionModel> g_mention_model;

    MentionModel::MentionModel(QObject * _parent)
        : CustomAbstractListModel(_parent)
        , messagesModel_(Logic::GetMessagesModel())
        , contactsModel_(Logic::getSearchModelForMentions())
    {
        connect(contactsModel_, &SearchModelDLG::results, this, &MentionModel::onContactsResults, Qt::QueuedConnection);
        connect(contactsModel_, &SearchModelDLG::emptyContactResults, this, &MentionModel::onContactsResults, Qt::QueuedConnection);
        connect(messagesModel_, &Logic::MessagesModel::ready, this, &MentionModel::onPageReady, Qt::QueuedConnection);
        connect(messagesModel_, &Logic::MessagesModel::updated, this, &MentionModel::onPageUpdated, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &MentionModel::onContactSelected, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageBuddies, this, &MentionModel::messageBuddies, Qt::QueuedConnection),
        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &MentionModel::avatarLoaded, Qt::QueuedConnection);

        match_.reserve(100);
    }

    int MentionModel::rowCount(const QModelIndex & _parent) const
    {
        return match_.size();
    }

    QVariant MentionModel::data(const QModelIndex & _index, int _role) const
    {
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)) || (unsigned)_index.row() >= match_.size())
            return QVariant();

        if (Testing::isAccessibleRole(_role))
            return match_[_index.row()].aimId_;

        return QVariant::fromValue<MentionModelItem>(match_[_index.row()]);
    }

    void MentionModel::filter()
    {
        if (searchPattern_.isEmpty())
            match_ = chatSenders_[dialogAimId_];
        else
        {
            match_.clear();

            std::copy_if(chatSenders_[dialogAimId_].begin(), chatSenders_[dialogAimId_].end(), std::back_inserter(match_),
                [this](auto& item)
            {
                return item.aimId_.contains(searchPattern_, Qt::CaseInsensitive)
                    || item.friendlyName_.contains(searchPattern_, Qt::CaseInsensitive);
            });
        }
        if (!match_.empty())
            emit dataChanged(index(0), index(match_.size()));

        contactsModel_->searchPatternChanged(searchPattern_);
    }

    void MentionModel::beforeShow()
    {
        contactsModel_->setTypingTimeout(0);
        setSearchPattern(QString());
        contactsModel_->resetTypingTimeout();
    }

    void MentionModel::setSearchPattern(const QString & _pattern)
    {
        searchPattern_ = _pattern;
        filter();
    }

    QString MentionModel::getSearchPattern() const
    {
        return searchPattern_;
    }

    void MentionModel::setDialogAimId(const QString& _aimid)
    {
        if (dialogAimId_ == _aimid)
            return;

        dialogAimId_ = _aimid;

        updateChatSenders(_aimid);
    }

    QString MentionModel::getDialogAimId() const
    {
        return dialogAimId_;
    }

    void MentionModel::emitChanged(const QModelIndex & _start, const QModelIndex & _end)
    {
        emit dataChanged(_start, _end);
    }

    bool MentionModel::isServiceItem(const QModelIndex & _index) const
    {
        if (!_index.isValid())
            return false;

        const auto row = _index.row();
        if (row < 0 || row >= match_.size())
            return false;

        return match_[row].isServiceItem();
    }

    void MentionModel::updateChatSenders(const QString& _chatAimId)
    {
        if (_chatAimId.isEmpty())
            return;

        const auto senders = messagesModel_->getSenders(_chatAimId, false);

        mentionItemVector newSenders;
        newSenders.reserve(senders.size());
        for (const auto& sender : senders)
            newSenders.emplace_back(sender.aimId_, sender.friendlyName_);

        if (newSenders.empty())
        {
            if (const auto contact = Logic::getContactListModel()->getContactItem(_chatAimId))
            {
                if (!contact->is_chat())
                    newSenders.emplace_back(_chatAimId, contact->Get()->GetDisplayName());
            }
        }

        chatSenders_[_chatAimId] = std::move(newSenders);
    }

    void MentionModel::onContactsResults()
    {
        if (contactsModel_->getCurrentPattern() == searchPattern_)
        {
            const auto contacts = contactsModel_->getMatch();
            if (!contacts.empty())
            {
                if (!match_.empty())
                {
                    if (!std::any_of(match_.begin(), match_.end(), [](const auto& mmi) { return mmi.isServiceItem(); }))
                    {
                        match_.emplace_back(MentionModelItem(qsl("~notinchat~"), QT_TRANSLATE_NOOP("mentions", "not in this chat")));
                    }
                }

                match_.reserve(match_.size() + contacts.size() + 1);
                for (const auto& dlg : contacts)
                {
                    if (!std::any_of(match_.begin(), match_.end(), [&dlg](const auto& mi) { return mi.aimId_ == dlg.AimId_; }))
                    {
                        const auto displayName = Logic::getContactListModel()->getDisplayName(dlg.AimId_);
                        match_.emplace_back(MentionModelItem(dlg.AimId_, displayName));
                    }
                }

                if (!match_.empty() && match_.back().isServiceItem())
                    match_.pop_back();
            }

            emit dataChanged(index(0), index(match_.size()));
            emit results();
        }
    }

    void MentionModel::onPageReady(const QString& _aimid)
    {
        setDialogAimId(_aimid);
    }

    void MentionModel::onPageUpdated(const QVector<Logic::MessageKey>& _list, const QString & _aimId, unsigned _mode)
    {
        updateChatSenders(_aimId);
    }

    void MentionModel::messageBuddies(const Data::MessageBuddies& /*_buddies*/, const QString & _aimId, Ui::MessagesBuddiesOpt /*_option*/, bool /*_havePending*/, qint64 /*_seq*/, int64_t /*_last_msgid*/)
    {
        updateChatSenders(_aimId);
    }

    void MentionModel::onContactSelected(const QString& _aimid)
    {
        if (!_aimid.isEmpty())
            setDialogAimId(_aimid);
    }

    void MentionModel::avatarLoaded(const QString & _aimid)
    {
        if (_aimid.isEmpty())
            return;

        int i = 0;
        for (const auto& iter : match_)
        {
            if (iter.aimId_ == _aimid)
            {
                const auto ndx = index(i);
                emitChanged(ndx, ndx);
                break;
            }
            ++i;
        }
    }

    Qt::ItemFlags MentionModel::flags(const QModelIndex &) const
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    MentionModel* GetMentionModel()
    {
        if (!g_mention_model)
            g_mention_model = std::make_unique<MentionModel>(nullptr);

        return g_mention_model.get();
    }

    void ResetMentionModel()
    {
        if (g_mention_model)
            g_mention_model.reset();
    }
}
