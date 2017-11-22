#include "stdafx.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "ContactListModel.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../main_window/MainWindow.h"

#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../MainPage.h"
#include "../MainWindow.h"
#include "../../utils/InterConnector.h"
#include "../../gui_settings.h"

namespace
{
	static const unsigned SORT_TIMEOUT = (build::is_debug() ? 120000 : 100);
}

namespace Logic
{
    std::unique_ptr<RecentsModel> g_recents_model;

	RecentsModel::RecentsModel(QObject *parent)
		: CustomAbstractListModel(parent)
		, Timer_(new QTimer(this))
        , FavoritesCount_(0)
        , FavoritesVisible_(true)
        , FavoritesHeadVisible_(true)
	{
        FavoritesVisible_ = Ui::get_gui_settings()->get_value(settings_favorites_visible, true);
        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &RecentsModel::contactChanged, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::activeDialogHide, this, &RecentsModel::activeDialogHide, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dlgStates, this, &RecentsModel::dlgStates, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &RecentsModel::contactChanged, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &RecentsModel::contactRemoved, Qt::QueuedConnection);
        Timer_->setSingleShot(true);
        connect(Timer_, &QTimer::timeout, this, &RecentsModel::sortDialogs);
	}

	int RecentsModel::rowCount(const QModelIndex &) const
	{
        if (Dialogs_.empty())
            return getSizeOfUnknownBlock();

        auto count = (int)Dialogs_.size() + getVisibleServiceItemInFavorites() + getSizeOfUnknownBlock();
        if (!FavoritesVisible_)
            count -= FavoritesCount_;

        return count;
	}

	QVariant RecentsModel::data(const QModelIndex &i, int r) const
	{
		if (!i.isValid() || (r != Qt::DisplayRole && !Testing::isAccessibleRole(r)))
			return QVariant();

		int cur = i.row();
		if (cur >= rowCount(i))
			return QVariant();

        if (cur == getUnknownsButtonIndex())
        {
            Data::DlgState st;
            st.AimId_ = qsl("~unknowns~");
            return QVariant::fromValue(st);
        }
        else if (cur == getFavoritesHeaderIndex())
        {
            Data::DlgState st;
            st.AimId_ = qsl("~favorites~");
            st.SetText(QT_TRANSLATE_NOOP("contact_list", "FAVORITES"));
            return QVariant::fromValue(st);
        }
        else if (cur == getRecentsHeaderIndex())
        {
            Data::DlgState st;
            st.AimId_ = qsl("~recents~");
            st.SetText(QT_TRANSLATE_NOOP("contact_list", "RECENTS"));
            return QVariant::fromValue(st);
        }

        cur = correctIndex(cur);

        if (cur >= (int) Dialogs_.size())
            return QVariant();

		const Data::DlgState& cont = Dialogs_[cur];

        if (Testing::isAccessibleRole(r))
            return cont.AimId_;

		return QVariant::fromValue(cont);
	}

	Qt::ItemFlags RecentsModel::flags(const QModelIndex &i) const
	{
		if (!i.isValid())
			return Qt::ItemIsEnabled;
		return QAbstractItemModel::flags(i) | Qt::ItemIsEnabled;
	}

    void RecentsModel::contactChanged(const QString& aimId)
    {
        int i = Indexes_[aimId];
        if (i != -1)
        {
            const auto idx = index(i);
            emit dataChanged(idx, idx);
        }
    }

	void RecentsModel::activeDialogHide(const QString& aimId)
	{
		Data::DlgState state;
		state.AimId_ = aimId;
		auto iter = std::find(Dialogs_.begin(), Dialogs_.end(), state);
		if (iter != Dialogs_.end())
        {
            if (iter->FavoriteTime_ != -1)
                --FavoritesCount_;

            QString hideContact = iter->AimId_;
            contactChanged(hideContact);
            Indexes_[iter->AimId_] = -1;
            Dialogs_.erase(iter);
            if (Logic::getContactListModel()->selectedContact() == aimId)
                Logic::getContactListModel()->setCurrent(QString(), -1, true);
            if (Dialogs_.empty() && !Logic::getUnknownsModel()->itemsCount())
                emit Utils::InterConnector::instance().showNoRecentsYet();

            emit updated();
		}
	}

	void RecentsModel::dlgStates(const QVector<Data::DlgState>& _states)
	{
        bool syncSort = false;
        for (const auto& _dlgState : _states)
        {
            const auto contactItem = Logic::getContactListModel()->getContactItem(_dlgState.AimId_);

            if (!contactItem)
                Logic::getContactListModel()->add(_dlgState.AimId_, _dlgState.Friendly_);

            if (!_dlgState.Official_ && !_dlgState.Chat_ && (!contactItem || (contactItem && contactItem->is_not_auth())))
                continue;

            auto iter = std::find(Dialogs_.begin(), Dialogs_.end(), _dlgState);
            if (iter != Dialogs_.end())
            {
                auto &existingDlgState = *iter;

                if (existingDlgState.FavoriteTime_ != _dlgState.FavoriteTime_)
                {
                    if (_dlgState.FavoriteTime_ != -1)
                        ++FavoritesCount_;
                    else
                        --FavoritesCount_;

                    emit favoriteChanged(_dlgState.AimId_);

                    syncSort = true;
                }

                if (existingDlgState.YoursLastRead_ != _dlgState.YoursLastRead_)
                {
                    emit readStateChanged(_dlgState.AimId_);
                }

                const auto existingText = existingDlgState.GetText();

                existingDlgState = _dlgState;

                const auto mustRecoverText = (!existingDlgState.HasText() && existingDlgState.HasLastMsgId());
                if (mustRecoverText)
                {
                    existingDlgState.SetText(existingText);
                }

                if (!syncSort)
                {
                    if (!Timer_->isActive())
                        Timer_->start(SORT_TIMEOUT);

                    int dist = (int)std::distance(Dialogs_.begin(), iter);
                    const auto idx = index(dist);
                    emit dataChanged(idx, idx);
                }
            }
            else if (!_dlgState.GetText().isEmpty() || _dlgState.FavoriteTime_ != -1)
            {
                if (_dlgState.FavoriteTime_ != -1)
                {
                    ++FavoritesCount_;
                    emit favoriteChanged(_dlgState.AimId_);
                }

                Dialogs_.push_back(_dlgState);

                if (_dlgState.FavoriteTime_ != -1)
                {
                    syncSort = true;
                }
                else
                {
                    if (!Timer_->isActive())
                        Timer_->start(SORT_TIMEOUT);
                }

                if (Dialogs_.size() == 1)
                {
                    emit Utils::InterConnector::instance().hideNoRecentsYet();
                }
            }

            Ui::MainWindow* w = Utils::InterConnector::instance().getMainWindow();

            if (_dlgState.AimId_ == Logic::getContactListModel()->selectedContact() && w && w->isActive() && w->isMainPage())
            {
                sendLastRead(_dlgState.AimId_);
            }
        }

        if (syncSort)
        {
            sortDialogs();
        }

        emit updated();
        emit dlgStatesHandled(_states);
	}

    void RecentsModel::unknownToRecents(const Data::DlgState& dlgState)
    {
        auto iter = std::find(Dialogs_.begin(), Dialogs_.end(), dlgState);
        if (iter == Dialogs_.end() && (!dlgState.GetText().isEmpty() || dlgState.FavoriteTime_ != -1))
        {
            if (dlgState.FavoriteTime_ != -1)
            {
                ++FavoritesCount_;
                emit favoriteChanged(dlgState.AimId_);
            }
            Dialogs_.push_back(dlgState);
            sortDialogs();
        }
    }

    bool RecentsModel::lessRecents(const QString& _aimid1, const QString& _aimid2)
    {
        auto first = getDlgState(_aimid1);
        auto second = getDlgState(_aimid2);

        if (first.FavoriteTime_ == -1 && second.FavoriteTime_ == -1)
            return first.Time_ > second.Time_;

        if (first.FavoriteTime_ == -1)
            return false;
        else if (second.FavoriteTime_ == -1)
            return true;

        if (first.FavoriteTime_ == second.FavoriteTime_)
            return first.AimId_ > second.AimId_;

        return first.FavoriteTime_ < second.FavoriteTime_;
    }

	void RecentsModel::sortDialogs()
	{
		std::sort(Dialogs_.begin(), Dialogs_.end(), [](const Data::DlgState& first, const Data::DlgState& second)
        {
            if (first.FavoriteTime_ == -1 && second.FavoriteTime_ == -1)
                return first.Time_ > second.Time_;

            if (first.FavoriteTime_ == -1)
                return false;
            else if (second.FavoriteTime_ == -1)
                return true;

            if (first.FavoriteTime_ == second.FavoriteTime_)
                return first.AimId_ > second.AimId_;

            return first.FavoriteTime_ < second.FavoriteTime_;

        });

		Indexes_.clear();
		int i = 0;
		for (const auto& iter : Dialogs_)
		{
			Indexes_[iter.AimId_] = i;
			++i;
		}

		emit dataChanged(index(0), index(rowCount()));
		emit orderChanged();
	}

    void RecentsModel::contactRemoved(const QString& _aimId)
    {
        activeDialogHide(_aimId);
    }

    void RecentsModel::refresh()
    {
        emit dataChanged(index(0), index(rowCount()));
    }

	Data::DlgState RecentsModel::getDlgState(const QString& aimId, bool fromDialog)
	{
		Data::DlgState state;
        auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), [&aimId](const Data::DlgState &item) { return item.AimId_ == aimId; });
		if (iter != Dialogs_.end())
			state = *iter;

		if (fromDialog)
			sendLastRead(aimId);

		return state;
	}

    void RecentsModel::toggleFavoritesVisible()
    {
        FavoritesVisible_ = !FavoritesVisible_;
        Ui::get_gui_settings()->set_value(settings_favorites_visible, FavoritesVisible_);
        emit dataChanged(index(0), index(rowCount()));
        emit updated();
    }

    void RecentsModel::unknownAppearance()
    {
        emit dataChanged(index(0), index(rowCount()));
    }

	void RecentsModel::sendLastRead(const QString& aimId)
	{
		Data::DlgState state;
		state.AimId_ = aimId.isEmpty() ? Logic::getContactListModel()->selectedContact() : aimId;
		std::vector<Data::DlgState>::iterator iter = std::find(Dialogs_.begin(), Dialogs_.end(), state);
		if (iter != Dialogs_.end() && (iter->UnreadCount_ != 0 || iter->YoursLastRead_ < iter->LastMsgId_))
		{
			iter->UnreadCount_ = 0;

			Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
			collection.set_value_as_qstring("contact", state.AimId_);
			collection.set_value_as_int64("message", iter->LastMsgId_);
			Ui::GetDispatcher()->post_message_to_core(qsl("dlg_state/set_last_read"), collection.get());

			int ind = (int)std::distance(Dialogs_.begin(), iter);
            const auto idx = index(ind);
			emit dataChanged(idx, idx);
			emit updated();
		}
	}

	void RecentsModel::markAllRead()
	{
		for (std::vector<Data::DlgState>::iterator iter = Dialogs_.begin(); iter != Dialogs_.end(); ++iter)
		{
			if (iter->UnreadCount_ != 0 || iter->YoursLastRead_ < iter->LastMsgId_)
			{
				Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
				collection.set_value_as_qstring("contact", iter->AimId_);
				collection.set_value_as_int64("message", iter->LastMsgId_);
				Ui::GetDispatcher()->post_message_to_core(qsl("dlg_state/set_last_read"), collection.get());

				int ind = (int)std::distance(Dialogs_.begin(), iter);
                const auto idx = index(ind);
				emit dataChanged(idx, idx);
				emit updated();
			}
		}
	}

	void RecentsModel::hideChat(const QString& aimId)
	{
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
		Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("contact", aimId);
		Ui::GetDispatcher()->post_message_to_core(qsl("dialogs/hide"), collection.get());
	}

	void RecentsModel::muteChat(const QString& aimId, bool mute)
	{
		Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("contact", aimId);
		collection.set_value_as_bool("mute", mute);
		Ui::GetDispatcher()->post_message_to_core(qsl("dialogs/mute"), collection.get());
	}

    bool RecentsModel::isFavorite(const QString& aimid) const
    {
        Data::DlgState state;
        state.AimId_ = aimid;
        std::vector<Data::DlgState>::const_iterator iter = std::find(Dialogs_.begin(), Dialogs_.end(), state);
        if (iter != Dialogs_.end())
            return iter->FavoriteTime_ != -1;

        return false;
    }

    bool RecentsModel::isServiceItem(const QModelIndex& i) const
    {
        auto row = i.row();
        return row == getUnknownsButtonIndex() || row == getFavoritesHeaderIndex() || row == getRecentsHeaderIndex();
    }

    bool RecentsModel::isFavoritesGroupButton(const QModelIndex& i) const
    {
        int r = i.row();
        return r == getFavoritesHeaderIndex();
    }

    bool RecentsModel::isFavoritesVisible() const
    {
        return FavoritesVisible_;
    }

    quint16 RecentsModel::getFavoritesCount() const
    {
        return FavoritesCount_;
    }

    bool RecentsModel::isUnknownsButton(const QModelIndex& i) const
    {
        int r = i.row();
        return r == getUnknownsButtonIndex();
    }

    bool RecentsModel::isRecentsHeader(const QModelIndex& i) const
    {
        int r = i.row();
        return r == getRecentsHeaderIndex();
    }

	QModelIndex RecentsModel::contactIndex(const QString& aimId) const
	{
		int i = 0;
		for (const auto& iter : Dialogs_)
		{
			if (iter.AimId_ == aimId)
            {
                break;
            }
			++i;
		}

        if (i < (int)Dialogs_.size())
        {
            if (FavoritesCount_)
            {
                if (i >= visibleContactsInFavorites())
                {
                    ++i;
                    if (!FavoritesVisible_)
                        i -= FavoritesCount_;
                }
                if (FavoritesHeadVisible_)
                    ++i;
            }
            else
            {
                ++i;
            }

            int result = i + getSizeOfUnknownBlock();
		    return index(result);
        }

		return QModelIndex();
	}

    QString RecentsModel::firstContact() const
    {
        if (!Dialogs_.empty())
        {
            return Dialogs_.front().AimId_;
        }
        return QString();
    }

	int RecentsModel::totalUnreads() const
	{
		int result = 0;
		for (auto iter : Dialogs_)
		{
			if (!Logic::getContactListModel()->isMuted(iter.AimId_))
				result += iter.UnreadCount_;
		}
		return result;
	}

    int RecentsModel::recentsUnreads() const
    {
        int result = 0;
        for (const auto& iter : Dialogs_)
        {
            if (!Logic::getContactListModel()->isMuted(iter.AimId_) && iter.FavoriteTime_ == -1)
                result += iter.UnreadCount_;
        }
        return result;
    }

    int RecentsModel::favoritesUnreads() const
    {
        int result = 0;
        for (const auto& iter : Dialogs_)
        {
            if (!Logic::getContactListModel()->isMuted(iter.AimId_) && iter.FavoriteTime_ != -1)
                result += iter.UnreadCount_;
        }
        return result;
    }

    QString RecentsModel::nextUnreadAimId() const
    {
        for (const auto& iter : Dialogs_)
        {
            if (!Logic::getContactListModel()->isMuted(iter.AimId_) &&
                iter.UnreadCount_ > 0)
            {
                return iter.AimId_;
            }
        }

        return QString();
    }

    QString RecentsModel::nextAimId(const QString& aimId) const
    {
        for (size_t i = 0; i < Dialogs_.size(); i++)
        {
            const Data::DlgState& iter = Dialogs_.at(i);
            if (iter.AimId_ == aimId && i < Dialogs_.size() - 1)
                return Dialogs_.at(i + 1).AimId_;
        }

        return QString();
    }

    QString RecentsModel::prevAimId(const QString& aimId) const
    {
        for (size_t i = 0; i < Dialogs_.size(); i++)
        {
            const Data::DlgState& iter = Dialogs_.at(i);
            if (iter.AimId_ == aimId && i > 0)
                return Dialogs_.at(i - 1).AimId_;
        }

        return QString();
    }

    int RecentsModel::correctIndex(int i) const
    {
        if (i == getUnknownsButtonIndex() || i == getFavoritesHeaderIndex() || i == getRecentsHeaderIndex())
            return i;

        if (FavoritesCount_ != 0)
        {
            if (i < getRecentsHeaderIndex())
                return i - getSizeOfUnknownBlock() - (FavoritesHeadVisible_ ? 1 : 0);

            i -= getVisibleServiceItemInFavorites();
            if (!FavoritesVisible_)
                i += FavoritesCount_;
        }
        else
        {
            --i;
        }

        return i - getSizeOfUnknownBlock();
    }

    int RecentsModel::visibleContactsInFavorites() const
    {
        return FavoritesVisible_ ? FavoritesCount_ : 0;
    }

    int RecentsModel::getUnknownsButtonIndex() const
    {
        return Logic::getUnknownsModel()->itemsCount() > 0 ? 0 : -1;
    }

    int RecentsModel::getSizeOfUnknownBlock() const
    {
        return Logic::getUnknownsModel()->itemsCount() > 0 ? 1 : 0;
    }

    int RecentsModel::getFavoritesHeaderIndex() const
    {
        return FavoritesCount_ && FavoritesHeadVisible_ ? getSizeOfUnknownBlock() : -1;
    }

    int RecentsModel::getRecentsHeaderIndex() const
    {
        if (!FavoritesCount_)
            return getSizeOfUnknownBlock();

        return getSizeOfUnknownBlock() + visibleContactsInFavorites() + (FavoritesHeadVisible_ ? 1 : 0);
    }

    int RecentsModel::getVisibleServiceItemInFavorites() const
    {
        return FavoritesCount_ ? (FavoritesHeadVisible_ ? 2 : 1) : 1;
    }

    void RecentsModel::setFavoritesHeadVisible(bool _isVisible)
    {
        FavoritesHeadVisible_ = _isVisible;
    }

    bool RecentsModel::isServiceAimId(const QString& _aimId) const
    {
        return _aimId.startsWith(ql1c('~')) && _aimId.endsWith(ql1c('~'));
    }

    std::vector<QString> RecentsModel::getSortedRecentsContacts() const
    {
        std::vector<QString> result;
        result.reserve(Dialogs_.size());
        for (const auto& item : Dialogs_)
            result.push_back(item.AimId_);

        return result;
    }

    RecentsModel* getRecentsModel()
    {
        if (!g_recents_model)
            g_recents_model = std::make_unique<RecentsModel>(nullptr);

        return g_recents_model.get();
    }

    void ResetRecentsModel()
    {
        g_recents_model.reset();
    }
}
