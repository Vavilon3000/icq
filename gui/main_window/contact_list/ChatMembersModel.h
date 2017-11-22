#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"
#include "../../types/chat.h"
#include "../../core_dispatcher.h"
#include "ContactItem.h"

namespace core
{
    struct icollection;
    enum class group_chat_info_errors;
}

namespace Data
{
    class ChatMemberInfo;
}

namespace Logic
{
    const int InitMembersLimit = 20;
    const int MaxMembersLimit = 1000;

    class ChatMembersModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void results();

    public Q_SLOTS:
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);

    private Q_SLOTS:
        void avatarLoaded(const QString&);
        void chatBlocked(const QVector<Data::ChatMemberInfo>&);
        void chatPending(const QVector<Data::ChatMemberInfo>&);
        void chatInfoFailed(qint64, core::group_chat_info_errors);

    public:
        ChatMembersModel(QObject* _parent);
        ChatMembersModel(const std::shared_ptr<Data::ChatInfo>& _info, QObject* _parent);

        ~ChatMembersModel();

        int rowCount(const QModelIndex& _parent = QModelIndex()) const;
        QVariant data(const QModelIndex& _index, int _role) const;
        Qt::ItemFlags flags(const QModelIndex& _index) const;
        void setSelectEnabled(bool _value);

        const Data::ChatMemberInfo* getMemberItem(const QString& _aimId) const;
        int getMembersCount() const;
        void loadAllMembers();
        void loadAllMembers(const QString& _aimId, int _count);
        void initForSingle(const QString& _aimId);
        void adminsOnly();
        void loadBlocked();
        void loadPending();
        bool isFullListLoaded_;
        bool isShortView_;

        bool isContactInChat(const QString& _aimId) const;
        QString getChatAimId() const;
        void updateInfo(const std::shared_ptr<Data::ChatInfo> &_info, bool _isAllChatInfo);
        void updateInfo(qint64 _seq, const std::shared_ptr<Data::ChatInfo> &_info, bool _isAllChatInfo);

        template<class Obj>
        static qint64 loadAllMembers(const QString& _aimId, int _limit, const Obj* _recv)
        {
            connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, _recv, &Obj::chatInfo, Qt::UniqueConnection);
            connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfoFailed, _recv, &Obj::chatInfoFailed, Qt::UniqueConnection);
            return loadAllMembersImpl(_aimId, _limit);
        }

        template<class Obj>
        static bool receiveMembers(qint64 _sendSeq, qint64 _seq, const Obj* _recv)
        {
            if (_seq != _sendSeq)
                return false;
            disconnect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, _recv, &Obj::chatInfo);
            disconnect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfoFailed, _recv, &Obj::chatInfoFailed);
            return true;
        }
        unsigned getVisibleRowsCount() const;

        static int get_limit(int limit);

        bool isAdmin() const;
        bool isModer() const;
        void clear();

        const std::vector<Data::ChatMemberInfo>& getMembers() const { return members_; }

    private:
        static qint64 loadAllMembersImpl(const QString& _aimId, int _limit);

    private:
        enum class mode
        {
            all,
            admins
        };

        mutable std::vector<Data::ChatMemberInfo>   members_;
        int                                         membersCount_;
        std::shared_ptr<Data::ChatInfo>             info_;
        QString                                     AimId_;
        qint64                                      chatInfoSequence_;
        QString                                     YourRole_;
        bool                                        selectEnabled_;
        bool                                        single_;

        mode                                        mode_;

        friend class SearchMembersModel;
    };

    void updateIgnoredModel(const QVector<QString>& _ignoredList);
    ChatMembersModel* getIgnoreModel();
}
