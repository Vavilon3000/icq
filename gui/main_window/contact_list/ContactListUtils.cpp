#include "stdafx.h"
#include "ContactListUtils.h"
#include "ContactListModel.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "SearchModelDLG.h"
#include "../../core_dispatcher.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"

namespace Logic
{
    bool is_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::MEMBERS_LIST || _regim == Logic::MembersWidgetRegim::IGNORE_LIST;
    }

    bool is_admin_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::ADMIN_MEMBERS;
    }

    bool is_select_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS
            || _regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE;
    }

    bool is_video_conference_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE;
    }

    QString aimIdFromIndex(const QModelIndex& _index, Logic::MembersWidgetRegim _regim)
    {
        QString aimid;
        if (Logic::is_members_regim(_regim) || Logic::is_video_conference_regim(_regim))
        {
            auto cont = _index.data().value<Data::ChatMemberInfo*>();
            if (!cont)
                return QString();
            aimid = cont->AimId_;
        }
        else if (qobject_cast<const Logic::UnknownsModel*>(_index.model()))
        {
            Data::DlgState dlg = _index.data().value<Data::DlgState>();
            aimid = dlg.AimId_;
        }
        else if (qobject_cast<const Logic::RecentsModel*>(_index.model()))
        {
            Data::DlgState dlg = _index.data().value<Data::DlgState>();
            aimid = Logic::getRecentsModel()->isServiceAimId(dlg.AimId_) ? QString() : dlg.AimId_;
        }
        else if (qobject_cast<const Logic::ContactListModel*>(_index.model()))
        {
            Data::Contact* cont = _index.data().value<Data::Contact*>();
            if (!cont)
                return QString();
            aimid = cont->AimId_;
        }
        else if (qobject_cast<const Logic::SearchModelDLG*>(_index.model()))
        {
            auto dlg = _index.data().value<Data::DlgState>();
            aimid = dlg.AimId_;
        }

        return aimid;
    }

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimid)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        result[qsl("contact")] = _aimid;
        return result;
    }

    void showContactListPopup(QAction* _action)
    {
        const auto params = _action->data().toMap();
        const QString command = params[qsl("command")].toString();
        const QString aimId = params[qsl("contact")].toString();

        if (command == ql1s("recents/mark_read"))
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_read);
            Logic::getRecentsModel()->sendLastRead(aimId);
        }
        else if (command == ql1s("recents/mute"))
        {
            Logic::getRecentsModel()->muteChat(aimId, true);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mute_recents_menu);
        }
        else if (command == ql1s("recents/unmute"))
        {
            Logic::getRecentsModel()->muteChat(aimId, false);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unmute);
        }
        else if (command == ql1s("recents/ignore") || command == ql1s("contacts/ignore"))
        {
            if (Logic::getContactListModel()->ignoreContactWithConfirm(aimId))
                Ui::GetDispatcher()->post_stats_to_core(command == ql1s("recents/ignore")
                    ? core::stats::stats_event_names::ignore_recents_menu : core::stats::stats_event_names::ignore_cl_menu);
        }
        else if (command == ql1s("recents/close"))
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_close);
            Logic::getRecentsModel()->hideChat(aimId);
        }
        else if (command == ql1s("recents/read_all"))
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_readall);
            Logic::getRecentsModel()->markAllRead();
        }
        else if (command == ql1s("contacts/call"))
        {
            Ui::GetDispatcher()->getVoipController().setStartV(aimId.toUtf8(), false);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::call_from_cl_menu);
        }
        else if (command == ql1s("contacts/Profile"))
        {
            emit Utils::InterConnector::instance().profileSettingsShow(aimId);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_cl);
        }
        else if (command == ql1s("contacts/spam"))
        {
            if (Logic::getContactListModel()->blockAndSpamContact(aimId))
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_cl_menu);
            }
        }
        else if (command == ql1s("contacts/remove"))
        {
            const QString text = QT_TRANSLATE_NOOP("popup_window", Logic::getContactListModel()->isChat(aimId)
                ? "Are you sure you want to leave chat?" : "Are you sure you want to delete contact?");

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                QT_TRANSLATE_NOOP("popup_window", "YES"),
                text,
                Logic::getContactListModel()->getDisplayName(aimId),
                nullptr
            );

            if (confirm)
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_cl_menu);
            }
        }
        else if (command == ql1s("recents/favorite"))
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", aimId);
            Ui::GetDispatcher()->post_message_to_core(qsl("favorite"), collection.get());
        }
        else if (command == ql1s("recents/unfavorite"))
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", aimId);
            Ui::GetDispatcher()->post_message_to_core(qsl("unfavorite"), collection.get());
        }
    }
}