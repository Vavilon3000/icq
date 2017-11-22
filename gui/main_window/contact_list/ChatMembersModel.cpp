#include "stdafx.h"
#include "ChatMembersModel.h"

#include "ContactListModel.h"
#include "SearchMembersModel.h"
#include "../../my_info.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"

namespace Logic
{
    ChatMembersModel::ChatMembersModel(QObject * /*parent*/)
        : ChatMembersModel(nullptr, nullptr)
    {}

    ChatMembersModel::ChatMembersModel(const std::shared_ptr<Data::ChatInfo>& _info, QObject *_parent)
        : CustomAbstractListModel(_parent)
        , isFullListLoaded_(false)
        , isShortView_(true)
        , membersCount_(0)
        , chatInfoSequence_(0)
        , selectEnabled_(true)
        , single_(false)
        , mode_(mode::all)
    {
        updateInfo(_info, false);
        connect(GetAvatarStorage(), &AvatarStorage::avatarChanged, this, &ChatMembersModel::avatarLoaded, Qt::QueuedConnection);
    }

    ChatMembersModel::~ChatMembersModel()
    {
    }

    unsigned int ChatMembersModel::getVisibleRowsCount() const
    {
        int currentCount = (int)members_.size();

        if (isShortView_)
            currentCount = std::min(Logic::InitMembersLimit, currentCount);
        return currentCount;
    }

    int ChatMembersModel::get_limit(int limit)
    {
        return std::max(limit, 0);
        /*
        if (limit > InitMembersLimit)
            return limit;

        return InitMembersLimit;
        */
    }

    int ChatMembersModel::rowCount(const QModelIndex &) const
    {
        return (int)getVisibleRowsCount();
    }

    QVariant ChatMembersModel::data(const QModelIndex & _ind, int _role) const
    {
        int currentCount = getVisibleRowsCount();

        if (!_ind.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)) || _ind.row() >= currentCount)
            return QVariant();
        Data::ChatMemberInfo* ptr = &(members_[_ind.row()]);

        if (Testing::isAccessibleRole(_role))
            return ptr->AimId_;

        return QVariant::fromValue<Data::ChatMemberInfo*>(ptr);
    }

    Qt::ItemFlags ChatMembersModel::flags(const QModelIndex &) const
    {
        Qt::ItemFlags flags = Qt::ItemIsEnabled;
        if (selectEnabled_)
            return flags |= Qt::ItemIsSelectable;

        return flags;
    }

    void ChatMembersModel::setSelectEnabled(bool _value)
    {
        selectEnabled_ = _value;
    }

    const Data::ChatMemberInfo* ChatMembersModel::getMemberItem(const QString& _aimId) const
    {
        for (const auto& item : members_)
        {
            if (item.AimId_ == _aimId)
                return &item;
        }

        return nullptr;
    }

    int ChatMembersModel::getMembersCount() const
    {
        return membersCount_;
    }

    void ChatMembersModel::chatInfo(qint64 _seq, std::shared_ptr<Data::ChatInfo> _info)
    {
        if (single_)
            return;

        if (receiveMembers(chatInfoSequence_, _seq, this))
        {
            updateInfo(_seq, _info, true);
            isFullListLoaded_ = (int(members_.size()) == membersCount_);
            emit dataChanged(index(0), index((int)members_.size()));
            emit results();
        }
    }

    void ChatMembersModel::avatarLoaded(const QString& _aimId)
    {
        int i = 0;
        for (const auto& iter : members_)
        {
            if (iter.AimId_ == _aimId)
            {
                const auto idx = index(i);
                emit dataChanged(idx, idx);
                break;
            }
            ++i;
        }
    }

    void ChatMembersModel::chatBlocked(const QVector<Data::ChatMemberInfo>& _blocked)
    {
        members_.assign(_blocked.begin(), _blocked.end());
        emit dataChanged(index(0), index(members_.size()));
    }

    void ChatMembersModel::chatPending(const QVector<Data::ChatMemberInfo>& _pending)
    {
        members_.assign(_pending.begin(), _pending.end());
        emit dataChanged(index(0), index(members_.size()));
    }

    void ChatMembersModel::chatInfoFailed(qint64 _id, core::group_chat_info_errors _e)
    {
        qDebug() << "can not load chat info" << _id << int(_e);
    }

    void ChatMembersModel::loadAllMembers(const QString& _aimId, int _count)
    {
        single_ = false;
        chatInfoSequence_ = loadAllMembers(_aimId, _count, this);
    }

    void ChatMembersModel::adminsOnly()
    {
        mode_ = mode::admins;
        const auto end = members_.end();
        const auto it = std::remove_if(members_.begin(), end, [](const auto& x) { return x.Role_ != ql1s("admin") && x.Role_ != ql1s("moder"); });
        members_.erase(it, end);
        isFullListLoaded_ = false;

        emit dataChanged(index(0), index(members_.size()));
    }

    void ChatMembersModel::loadBlocked()
    {
        const auto idx = index(0);
        emit dataChanged(idx, idx);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatBlocked, this, &ChatMembersModel::chatBlocked, Qt::UniqueConnection);
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", AimId_);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/blocked/get"), collection.get());
    }

    void ChatMembersModel::loadPending()
    {
        const auto idx = index(0);
        emit dataChanged(idx, idx);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatPending, this, &ChatMembersModel::chatPending, Qt::UniqueConnection);
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", AimId_);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/pending/get"), collection.get());
    }

    void ChatMembersModel::initForSingle(const QString& _aimId)
    {
        single_ = true;
        mode_ = mode::all;
        members_.clear();
        Data::ChatMemberInfo info;
        info.AimId_ = _aimId;
        info.NickName_ = Logic::getContactListModel()->getDisplayName(_aimId);
        members_.push_back(info);
        info.AimId_ = Ui::MyInfo()->aimId();
        info.NickName_ = Ui::MyInfo()->friendlyName();
        members_.push_back(std::move(info));
        emit dataChanged(index(0), index(1));
    }

    void ChatMembersModel::loadAllMembers()
    {
        single_ = false;
        chatInfoSequence_ = loadAllMembers(AimId_, 0, this);
    }

    qint64 ChatMembersModel::loadAllMembersImpl(const QString& _aimId, int _limit)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", _aimId);
        collection.set_value_as_int("limit", get_limit(_limit));
        return Ui::GetDispatcher()->post_message_to_core(qsl("chats/info/get"), collection.get());
    }

    void ChatMembersModel::updateInfo(const std::shared_ptr<Data::ChatInfo>& _info, bool _isAllChatInfo)
    {
        if (!_info)
            return;
        AimId_ = _info->AimId_;
        YourRole_ = _info->YourRole_;
        membersCount_ = _info->MembersCount_;

        members_.assign(_info->Members_.cbegin(), _info->Members_.cend());
        if (mode_ == mode::admins)
            adminsOnly();
        else
            emit dataChanged(index(0), index(members_.size()));

        isFullListLoaded_ = membersCount_ <= InitMembersLimit;

        if (_isAllChatInfo)
            isFullListLoaded_ = _isAllChatInfo;
    }

    void ChatMembersModel::updateInfo(qint64 _seq, const std::shared_ptr<Data::ChatInfo> &_info, bool _isAllChatInfo)
    {
        if (_seq < chatInfoSequence_)
            return;
        updateInfo(_info, _isAllChatInfo);
    }

    bool ChatMembersModel::isContactInChat(const QString& _aimId) const
    {
        // TODO : use hash-table here
        return std::any_of(members_.cbegin(), members_.cend(), [&_aimId](const auto& x) { return x.AimId_ == _aimId; });
    }

    QString ChatMembersModel::getChatAimId() const
    {
        return AimId_;
    }

    bool ChatMembersModel::isAdmin() const
    {
        return YourRole_ == ql1s("admin");
    }

    bool ChatMembersModel::isModer() const
    {
        return YourRole_ == ql1s("moder");
    }

    void ChatMembersModel::clear()
    {
        mode_ = mode::all;
        const int size = members_.size();
        members_.clear();
        emit dataChanged(index(0), index(size));
    }

    void updateIgnoredModel(const QVector<QString>& _ignoredList)
    {
        auto info = std::make_shared<Data::ChatInfo>();
        info->MembersCount_ = _ignoredList.size();
        info->Members_.reserve(info->MembersCount_);
        for (const auto& aimId : _ignoredList)
        {
            Data::ChatMemberInfo member;
            member.AimId_ = aimId;
            info->Members_.append(std::move(member));
        }

        Logic::getContactListModel()->removeContactsFromModel(_ignoredList);

        getIgnoreModel()->updateInfo(info, true);
        emit getIgnoreModel()->results();
    }

    ChatMembersModel* getIgnoreModel()
    {
        static auto ignoreModel = std::make_unique<ChatMembersModel>(std::make_shared<Data::ChatInfo>(), nullptr);
        ignoreModel->setSelectEnabled(false);
        return ignoreModel.get();
    }
}
