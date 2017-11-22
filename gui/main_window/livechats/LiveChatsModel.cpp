#include "stdafx.h"
#include "LiveChatsModel.h"

#include "../../core_dispatcher.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

namespace
{
    const int preload_size = 30;
}

namespace Logic
{
    static std::unique_ptr<LiveChatsModel> gLivechatModel;

    LiveChatsModel::LiveChatsModel(QObject* _parent)
        : QAbstractListModel(_parent)
        , requestedTag_(qsl("_"))
        , finished_(false)
        , inited_(false)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatsHome, this, &LiveChatsModel::chatsHome, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatsHomeError, this, &LiveChatsModel::chatsHomeError, Qt::QueuedConnection);
        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &LiveChatsModel::avatarLoaded, Qt::QueuedConnection);
    }

    void LiveChatsModel::initIfNeeded()
    {
        if (!inited_)
        {
            inited_ = true;
            requestMore();
        }
    }

    void LiveChatsModel::avatarLoaded(const QString& _aimId)
    {
        if (!inited_)
            return;

        int i = 0;
        for (const auto& iter : Utils::as_const(cache_))
        {
            if (iter.AimId_ == _aimId)
            {
                const auto idx = index(i);
                emit dataChanged(idx, idx);
                return;
            }

            int memberCount = 0;
            for (const auto& member : iter.Members_)
            {
                if (member.AimId_ == _aimId)
                {
                    const auto idx = index(i);
                    emit dataChanged(idx, idx);
                    return;
                }

                if (++memberCount > 3)
                    break;
            }

            ++i;
        }
    }

    int LiveChatsModel::rowCount(const QModelIndex &/*parent*/) const
    {
        return cache_.size();
    }

    QVariant LiveChatsModel::data(const QModelIndex& _index, int _role) const
    {
        int row = _index.row();
        if (row < 0 || row > rowCount())
            return QVariant();

        if (_role == Qt::DisplayRole && rowCount() - _index.row() <= preload_size)
            requestMore();

        return QVariant::fromValue(cache_.at(row));
    }

    Qt::ItemFlags LiveChatsModel::flags(const QModelIndex& _index) const
    {
        if (!_index.isValid())
            return Qt::ItemIsEnabled;
        return QAbstractItemModel::flags(_index) | Qt::ItemIsEnabled;
    }

    void LiveChatsModel::select(const QModelIndex& _index)
    {
        int row = _index.row();
        if (row < 0 || row > rowCount())
            return;

        emit selected(cache_[row]);
        emit Utils::InterConnector::instance().liveChatSelected();
    }

    void LiveChatsModel::pending(const QString& _aimId)
    {
        for (auto iter = cache_.begin(); iter != cache_.end(); ++iter)
        {
            if (iter->AimId_ == _aimId)
            {
                iter->YouPending_ = true;
                break;
            }
        }
    }

    void LiveChatsModel::joined(const QString& _aimId)
    {
        for (auto iter = cache_.begin(); iter != cache_.end(); ++iter)
        {
            if (iter->AimId_ == _aimId)
            {
                iter->YouMember_ = true;
                break;
            }
        }
    }

    void LiveChatsModel::chatsHome(const QVector<Data::ChatInfo>& _chats, const QString& _newTag, bool _restart, bool _finished)
    {
        tag_ = _newTag;
        finished_ = _finished;

        if (_restart)
        {
            cache_.clear();
            tag_.clear();
            finished_ = false;
            requestMore();
            emit dataChanged(index(0), index(0));
            return;
        }

        for (const auto& iter : _chats)
            cache_.append(iter);

        emit dataChanged(index(0), index(rowCount()));
    }

    void LiveChatsModel::chatsHomeError(int)
    {
        requestMore();
    }

    void LiveChatsModel::requestMore() const
    {
        if (tag_ == requestedTag_ || finished_)
            return;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("tag", tag_);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/home/get"), collection.get());
        requestedTag_ = tag_;
    }

    LiveChatsModel* GetLiveChatsModel()
    {
        if (!gLivechatModel)
            gLivechatModel = std::make_unique<LiveChatsModel>(nullptr);

        return gLivechatModel.get();
    }

    void ResetLiveChatsModel()
    {
        if (gLivechatModel)
            gLivechatModel.reset();
    }
}