#pragma once

#include "CustomAbstractListModel.h"
#include "../history_control/MessagesModel.h"

namespace Logic
{
    class ChatMembersModel;
    class SearchModelDLG;

    struct MentionModelItem
    {
        QString aimId_;
        QString friendlyName_;

        MentionModelItem() = default;

        MentionModelItem(const QString& _aimid, const QString& _friendly)
            : aimId_(_aimid), friendlyName_(_friendly)
        {}

        bool operator==(const MentionModelItem& _other) const
        {
            return aimId_ == _other.aimId_;
        }

        bool isServiceItem() const
        {
            return aimId_.startsWith(ql1c('~'));
        }
    };

    typedef std::vector<MentionModelItem> mentionItemVector;

    class MentionModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void mentionSelected(const QString& _aimid, const QString& _friendly);
        void results();

    public Q_SLOTS:
        void filter();
        void beforeShow();

    private Q_SLOTS:
        void onContactsResults();
        void onPageReady(const QString& _aimid);
        void onPageUpdated(const QVector<Logic::MessageKey>& _list, const QString& _aimId, unsigned _mode);
        void messageBuddies(const Data::MessageBuddies& _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, int64_t _last_msgid);
        void onContactSelected(const QString&);
        void avatarLoaded(const QString& _aimid);

    public:
        MentionModel(QObject* _parent);

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        Qt::ItemFlags flags(const QModelIndex &) const override;

        void setSearchPattern(const QString& _pattern);
        QString getSearchPattern() const;

        void setDialogAimId(const QString& _aimid);
        QString getDialogAimId() const;

        void emitChanged(const QModelIndex& _start, const QModelIndex& _end);

        bool isServiceItem(const QModelIndex& _index) const;

    private:
        void updateChatSenders(const QString& _chatAimId);

    private:
        MessagesModel* messagesModel_;
        SearchModelDLG* contactsModel_;

        QString searchPattern_;
        QString dialogAimId_;

        mentionItemVector match_;
        std::map<QString, mentionItemVector> chatSenders_;
    };

    MentionModel* GetMentionModel();
    void ResetMentionModel();
}

Q_DECLARE_METATYPE(Logic::MentionModelItem);