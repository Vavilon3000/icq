#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"
#include "../../types/chat.h"

#include "ContactItem.h"

namespace core
{
    struct icollection;
}

namespace Ui
{
    class HistoryControlPage;
}

namespace Logic
{
    namespace ContactListSorting
    {
        const int maxTopContactsByOutgoing = 7;
        typedef std::function<bool (const Logic::ContactItem&, const Logic::ContactItem&)> contact_sort_pred;

        struct ItemLessThanDisplayName
        {
            inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
            {
                const auto& firstName = _first.Get()->GetDisplayName();
                const auto& secondName = _second.Get()->GetDisplayName();

                return firstName.compare(secondName, Qt::CaseInsensitive) < 0;
            }
        };

        struct ItemLessThan
        {
            inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
            {
                if (_first.Get()->GroupId_ == _second.Get()->GroupId_)
                {
                    if (_first.is_group() && _second.is_group())
                        return false;

                    if (_first.is_group())
                        return true;

                    if (_second.is_group())
                        return false;

                    return ItemLessThanDisplayName()(_first, _second);
                }

                return _first.Get()->GroupId_ < _second.Get()->GroupId_;
            }
        };

        struct ItemLessThanNoGroups
        {
            ItemLessThanNoGroups(const QDateTime& _current)
                : current_(_current)
            {
            }


            inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
            {
                if (_first.Get()->IsChecked_ != _second.Get()->IsChecked_)
                    return _first.Get()->IsChecked_;

                const auto firstIsActive = _first.is_active(current_);
                const auto secondIsActive = _second.is_active(current_);
                if (firstIsActive != secondIsActive)
                    return firstIsActive;

                return ItemLessThanDisplayName()(_first, _second);
            }

            QDateTime current_;
        };

        struct ItemLessThanSelectMembers
        {
            ItemLessThanSelectMembers(const QDateTime& _current)
                : current_(_current)
            {
            }

            inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
            {
                if (_first.Get()->IsChecked_ != _second.Get()->IsChecked_)
                    return _first.Get()->IsChecked_;

                const auto firstIsActive = _first.is_active(current_);
                const auto secondIsActive = _second.is_active(current_);
                if (firstIsActive != secondIsActive)
                    return firstIsActive;

                return ItemLessThanDisplayName()(_first, _second);
            }

            QDateTime current_;
        };
    }

    class contact_profile;
    typedef std::shared_ptr<contact_profile> profile_ptr;

    enum class UpdateChatSelection
    {
        No,
        Yes
    };

    class ContactListModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void currentDlgStateChanged() const;
        void selectedContactChanged(const QString&);
        void contactChanged(const QString&) const;
        void select(const QString&, qint64, qint64 quote_id = -1, Logic::UpdateChatSelection mode = Logic::UpdateChatSelection::No) const;
        void profile_loaded(profile_ptr _profile) const;
        void contact_added(const QString& _contact, bool _result);
        void contact_removed(const QString& _contact);
        void leave_dialog(const QString& _contact);
        void results();
        void needSwitchToRecents();
        void needJoinLiveChat(const QString& _stamp);
        void liveChatJoined(const QString&);
        void liveChatRemoved(const QString&);
        void switchTab(const QString&);
		void ignore_contact(const QString&);
        void youRoleChanged(const QString&);

    private Q_SLOTS:
        void contactList(std::shared_ptr<Data::ContactList>, const QString&);
        void avatarLoaded(const QString&);
        void presence(std::shared_ptr<Data::Buddy>);
        void contactRemoved(const QString&);
        void outgoingMsgCount(const QString& _aimid, const int _count);

    public Q_SLOTS:
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);
        void sort();
        void forceSort();
        void scrolled(int);
        void groupClicked(int);

    public:
        explicit ContactListModel(QObject* _parent);

        virtual QVariant data(const QModelIndex& _index, int _role) const override;
        virtual Qt::ItemFlags flags(const QModelIndex& _index) const override;
        virtual int rowCount(const QModelIndex& _parent = QModelIndex()) const override;

        ContactItem* getContactItem(const QString& _aimId);
        void setCurrentCallbackHappened(Ui::HistoryControlPage* _page);

        void setFocus();
        void setCurrent(const QString& _aimId, qint64 id, bool _select = false, std::function<void(Ui::HistoryControlPage*)> _getPageCallback = nullptr, qint64 quote_id = -1);

        const ContactItem* getContactItem(const QString& _aimId) const;

        QString selectedContact() const;
        QString selectedContactName() const;

        void add(const QString& _aimId, const QString& _friendly);

        void setContactVisible(const QString& _aimId, bool _visible);

        QString getDisplayName(const QString& _aimId) const;
        QString getInputText(const QString& _aimId) const;
        void setInputText(const QString& _aimId, const QString& _text);
        QDateTime getLastSeen(const QString& _aimId) const;
        QString getLastSeenString(const QString& _aimId) const;
        QString getState(const QString& _aimId) const;
        QString getStatusString(const QString& _aimId) const;
        bool isChat(const QString& _aimId) const;
        bool isMuted(const QString& _aimId) const;
        bool isLiveChat(const QString& _aimId) const;
        bool isOfficial(const QString& _aimId) const;
        bool isNotAuth(const QString& _aimId) const;
        QModelIndex contactIndex(const QString& _aimId) const;

        void addContactToCL(const QString& _aimId, std::function<void(bool)> _callBack = [](bool) {});
        bool blockAndSpamContact(const QString& _aimId, bool _withConfirmation = true);
        void getContactProfile(const QString& _aimId, std::function<void(profile_ptr, int32_t)> _callBack = [](profile_ptr, int32_t) {});
        void ignoreContact(const QString& _aimId, bool _ignore);
        bool ignoreContactWithConfirm(const QString& _aimId);
        bool isYouAdmin(const QString& _aimId);
        QString getYourRole(const QString& _aimId);
        void setYourRole(const QString& _aimId, const QString& _role);
        void removeContactFromCL(const QString& _aimId);
        void renameChat(const QString& _aimId, const QString& _friendly);
        void renameContact(const QString& _aimId, const QString& _friendly);
        void static getIgnoreList();

        void removeContactsFromModel(const QVector<QString>& _vcontacts);

        void emitChanged(const int _first, const int _last);

        std::vector<ContactItem> GetCheckedContacts() const;
        void clearChecked();
        void setChecked(const QString& _aimId, bool _isChecked);
        void setIsWithCheckedBox(bool);
        bool getIsChecked(const QString& _aimId) const;

        bool isWithCheckedBox();
        QString contactToTryOnTheme() const;
        void joinLiveChat(const QString& _stamp, bool _silent);

        void next();
        void prev();

        bool contains(const QString& _aimdId) const;
        void updatePlaceholders();

    private:
        std::shared_ptr<bool>	ref_;
        std::function<void(Ui::HistoryControlPage*)> gotPageCallback_;
        void rebuildIndex();
        void rebuildVisibleIndex();
        int addItem(Data::ContactPtr _contact, const bool _updatePlaceholder = true);
        void pushChange(int i);
        void processChanges();
        bool isVisibleItem(const ContactItem& _item) const;
        int getIndexByOrderedIndex(int _index) const;
        int getOrderIndexByAimid(const QString& _aimId) const;
        void updateSortedIndexesList(std::vector<int>& _list, ContactListSorting::contact_sort_pred _less);
        ContactListSorting::contact_sort_pred getLessFuncCL(const QDateTime& current) const;
        void updateIndexesListAfterRemoveContact(std::vector<int>& _list, int _index);
        int innerRemoveContact(const QString& _aimId);

        int getAbsIndexByVisibleIndex(const int& _visibleIndex) const;

        std::vector<ContactItem> contacts_;
        std::vector<int> sorted_index_cl_;
        std::vector<int> sorted_index_recents_;
        QHash<QString, int> indexes_;
        std::vector<int> visible_indexes_;
        bool sortNeeded_;
        QTimer* sortTimer_;

        int scrollPosition_;
        mutable int minVisibleIndex_;
        mutable int maxVisibleIndex_;
        std::vector<int> updatedItems_;

        QString currentAimId_;

        std::vector<ContactItem> match_;
        bool searchRequested_;
        bool isSearch_;
        bool isWithCheckedBox_;
    };

    ContactListModel* getContactListModel();
    void ResetContactListModel();
}

Q_DECLARE_METATYPE(Logic::UpdateChatSelection);
