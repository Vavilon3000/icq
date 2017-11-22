#pragma once

namespace Logic
{
    enum MembersWidgetRegim
    {
        CONTACT_LIST,
        SELECT_MEMBERS,
        MEMBERS_LIST,
        IGNORE_LIST,
        ADMIN_MEMBERS,
        SHARE_LINK,
        SHARE_TEXT,
        PENDING_MEMBERS,
        UNKNOWN,
        FROM_ALERT,
        HISTORY_SEARCH,
        VIDEO_CONFERENCE,
        CONTACT_LIST_POPUP,
    };

    bool is_members_regim(int _regim);
    bool is_admin_members_regim(int _regim);
    bool is_select_members_regim(int _regim);
    bool is_video_conference_regim(int _regim);

    QString aimIdFromIndex(const QModelIndex& _current, Logic::MembersWidgetRegim _regim);

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimid = QString());
    void showContactListPopup(QAction* _action);
}