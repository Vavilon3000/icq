#include "stdafx.h"
#include "SearchModelDLG.h"
#include "ContactListModel.h"
#include "ContactItem.h"
#include "RecentsModel.h"
#include "contact_profile.h"

#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../utils/InterConnector.h"
#include "../../../common.shared/common_defs.h"
#include "../../utils/log/log.h"

namespace
{
    static const int maxGlobalPreviewCount = 3;

    static const int TYPING_TIMEOUT = 200;
    static const int SORT_TIMEOUT = 100;
    static const int SORTED_RESULTS_SHOWING_DELAY = SORT_TIMEOUT * 1.5;
    static const int GLOBAL_RESULTS_TIMEOUT = 5000;
}

namespace Logic
{
	SearchModelDLG::SearchModelDLG(QObject *parent)
		: AbstractSearchModel(parent)

        , searchPatternsCount_(0)
        , LastRequestId_(-1)
        , contactsCount_(0)
        , messagesCount_(0)
        , previewGlobalContactsCount_(0)
        , timerSort_(new QTimer(this))
        , timerSearch_(new QTimer(this))
        , timerGlobalResults_(new QTimer(this))
        , timerSortedResultsShow_(new QTimer(this))
        , isSearchInDialog_(false)
        , ContactsOnly_(false)
        , searchInHistory_(true)
        , requestsReturnedCount_(0)
        , currentViewMode_(searchModel::searchView::results_page)
	{
		connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedMessage,    this, &SearchModelDLG::searchedMessage, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedContacts,   this, &SearchModelDLG::searchedContacts, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::emptySearchResults, this, &SearchModelDLG::emptySearchResults, Qt::QueuedConnection);

        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged,      this, &SearchModelDLG::avatarLoaded, Qt::QueuedConnection);
        connect(getContactListModel(), &Logic::ContactListModel::contact_removed, this, &SearchModelDLG::contactRemoved, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::disableSearchInDialog,  this, &SearchModelDLG::disableSearchInDialog, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::repeatSearch,           this, &SearchModelDLG::repeatSearch, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::globalHeaderBackClicked,this, &SearchModelDLG::switchToResultsPage, Qt::QueuedConnection);

        connect(this, &SearchModelDLG::morePeopleClicked, this, &SearchModelDLG::switchToAllPeoplePage, Qt::QueuedConnection);
        connect(this, &SearchModelDLG::moreChatsClicked,  this, &SearchModelDLG::switchToAllChatsPage,  Qt::QueuedConnection);

        auto initTimer = [this](auto _timer, auto _timeout, auto _slot)
        {
            _timer->setSingleShot(true);
            _timer->setInterval(_timeout);
            _timer->setTimerType(Qt::CoarseTimer);
            connect(_timer, &QTimer::timeout, this, _slot, Qt::QueuedConnection);
        };

        initTimer(timerSort_, SORT_TIMEOUT, &SearchModelDLG::sortDialogs);
        initTimer(timerSearch_, TYPING_TIMEOUT, &SearchModelDLG::search);
        initTimer(timerSortedResultsShow_, SORTED_RESULTS_SHOWING_DELAY, &SearchModelDLG::messageResultsTimeout);
        initTimer(timerGlobalResults_, GLOBAL_RESULTS_TIMEOUT, &SearchModelDLG::globalResultsTimeout);

        Match_.reserve(common::get_limit_search_results());
        tempMatch_.reserve(common::get_limit_search_results());
        globalContacts_.reserve(common::get_limit_search_results());
        globalChats_.reserve(common::get_limit_search_results());
        globalChatsData_.reserve(common::get_limit_search_results());
	}

    int SearchModelDLG::count() const
    {
        return (int)Match_.size()
            + (getMorePeopleButtonIndex() != -1 ? 1 : 0)
            + (getMoreChatsButtonIndex() != -1 ? 1 : 0)
            + (getMessagesHeaderIndex() != -1 ? 1 : 0)
            + (getPeopleHeaderIndex() != -1 ? 1 : 0);
    }

    int SearchModelDLG::contactsCount() const
    {
        return contactsCount_;
    }

	int SearchModelDLG::rowCount(const QModelIndex &) const
	{
		return count();
	}

	QVariant SearchModelDLG::data(const QModelIndex & _index, int _role) const
	{
		if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)))// || (unsigned)_index.row() >= Match_.size())
			return QVariant();

        int cur = _index.row();
		if (cur >= (int)rowCount(_index))
			return QVariant();

        if (cur == getMorePeopleButtonIndex())
        {
            Data::DlgState st;
            st.AimId_ = qsl("~more people~");
            st.SetText(QT_TRANSLATE_NOOP("contact_list", "More results"));
            return QVariant::fromValue(st);
        }
        else if (cur == getMoreChatsButtonIndex())
        {
            Data::DlgState st;
            st.AimId_ = qsl("~more chats~");
            st.SetText(QT_TRANSLATE_NOOP("contact_list", "Search for livechats and channels"));
            return QVariant::fromValue(st);
        }
        else if (cur == getMessagesHeaderIndex())
        {
            Data::DlgState st;
            st.AimId_ = qsl("~messages~");
            st.SetText(QT_TRANSLATE_NOOP("contact_list", "MESSAGES"));
            return QVariant::fromValue(st);
        }
        else if (cur == getPeopleHeaderIndex())
        {
            Data::DlgState st;
            st.AimId_ = qsl("~people~");
            st.SetText(QT_TRANSLATE_NOOP("contact_list", "PEOPLE"));
            return QVariant::fromValue(st);
        }

        const auto cIndex = correctIndex(_index.row());
        if (cIndex < 0 || cIndex >= Match_.size())
            return QVariant();

		return QVariant::fromValue(Match_[cIndex]);
	}

    int SearchModelDLG::correctIndex(const int i) const
    {
        if (isServiceItem(i))
            return i;

        auto shift = 0;
        const auto addShift = [&shift, i](const auto index)
        {
            if (index != -1 && i > index)
                shift++;
        };
        addShift(getMorePeopleButtonIndex());
        addShift(getMoreChatsButtonIndex());
        addShift(getMessagesHeaderIndex());
        addShift(getPeopleHeaderIndex());

        return i - shift;
    }

	Qt::ItemFlags SearchModelDLG::flags(const QModelIndex &) const
	{
		return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	}

	void SearchModelDLG::setFocus()
	{
		clear_match();
        searchQuery_.clear();
        LastRequestId_ = -1;
	}

    void SearchModelDLG::clear_match()
    {
        Match_.clear();
        topMessageKeys_.clear();
        globalContacts_.clear();
        globalChats_.clear();
        globalChatsData_.clear();

        contactsCount_ = 0;
        messagesCount_ = 0;
        previewGlobalContactsCount_ = 0;

        currentViewMode_ = searchModel::searchView::results_page;
        requestsReturnedCount_ = 0;
        lastGlobalKeyword_.clear();

        emit dataChanged(index(0), index(count()));
        emit Utils::InterConnector::instance().hideGlobalSearchHeader();
    }

    bool SearchModelDLG::getChatInfo(const QModelIndex & _index, Out Data::ChatInfo & _info) const
    {
        if (!_index.isValid() || globalChatsData_.empty())
            return false;

        const auto cIndex = correctIndex(_index.row());
        if (cIndex < 0 || cIndex >= Match_.size())
            return false;

        const auto& dlgState = Match_[cIndex];
        if (dlgState.Chat_ && dlgState.isFromGlobalSearch_)
        {
            auto it = std::find_if(
                globalChatsData_.begin(),
                globalChatsData_.end(),
                [&dlgState](const auto& chat) { return chat.AimId_ == dlgState.AimId_; });

            if (it != globalChatsData_.end())
            {
                _info = *it;
                return true;
            }
        }
        return false;
    }

    void SearchModelDLG::setSearchInHistory(const bool value)
    {
        searchInHistory_ = value;
    }

    bool SearchModelDLG::getSearchInHistory() const
    {
        return searchInHistory_;
    }

    void SearchModelDLG::setContactsOnly(const bool value)
    {
        ContactsOnly_ = value;
    }

    bool SearchModelDLG::getContactsOnly() const
    {
        return ContactsOnly_;
    }

    QString SearchModelDLG::getCurrentPattern() const
    {
        return searchQuery_;
    }

    int SearchModelDLG::getMessagesHeaderIndex() const
    {
        if (isSearchInDialog_ || currentViewMode_ != searchModel::searchView::results_page || topMessageKeys_.empty())
            return -1;

        return
            contactsCount_
            + previewGlobalContactsCount_
            + (getPeopleHeaderIndex() != -1 ? 1 : 0)
            + (getMorePeopleButtonIndex() != -1 ? 1 : 0)
            + (getMoreChatsButtonIndex()  != -1 ? 1 : 0);
    }

    int SearchModelDLG::getPeopleHeaderIndex() const
    {
        if (searchInHistory_ &&
            !isSearchInDialog_ &&
            currentViewMode_ == searchModel::searchView::results_page &&
            previewGlobalContactsCount_ > 0)
        {
            return contactsCount_;
        }

        return -1;
    }

    int SearchModelDLG::getMorePeopleButtonIndex() const
    {
        if (searchInHistory_ &&
            !isSearchInDialog_ &&
            currentViewMode_ == searchModel::searchView::results_page &&
            globalContacts_.size() > previewGlobalContactsCount_)
        {
            return
                contactsCount_
                + previewGlobalContactsCount_
                + (getPeopleHeaderIndex() != -1 ? 1 : 0);
        }

        return -1;
    }

    int SearchModelDLG::getMoreChatsButtonIndex() const
    {
        if (searchInHistory_ &&
            !isSearchInDialog_ &&
            currentViewMode_ == searchModel::searchView::results_page &&
            !globalChats_.empty())
        {
            return
                contactsCount_
                + previewGlobalContactsCount_
                + (getPeopleHeaderIndex() != -1 ? 1 : 0)
                + (globalContacts_.size() > previewGlobalContactsCount_ ? 1 : 0);
        }

        return -1;
    }

    bool SearchModelDLG::isResultFromGlobalSearch(const QModelIndex & _index) const
    {
        if (Match_.empty() || !_index.isValid() || isServiceItem(_index.row()))
            return false;

        if (currentViewMode_ == Logic::searchModel::searchView::results_page)
        {
            const auto cIndex = correctIndex(_index.row());
            if (cIndex < 0 || cIndex >= Match_.size())
                return false;

            const auto& dlgState = Match_[cIndex];
            if (!dlgState.isFromGlobalSearch_)
                return false;
        }
        return true;
    }


    bool SearchModelDLG::isServiceItem(int i) const
    {
        if (!searchInHistory_)
            return false;

        return i == getMorePeopleButtonIndex()
            || i == getMoreChatsButtonIndex()
            || i == getMessagesHeaderIndex()
            || i == getPeopleHeaderIndex();
    }

    bool SearchModelDLG::isServiceButton(const QModelIndex & _index) const
    {
        const auto row = _index.row();
        return row == getMorePeopleButtonIndex() || row == getMoreChatsButtonIndex();
    }

	void SearchModelDLG::searchPatternChanged(QString _newPattern)
	{
        searchQuery_ = std::move(_newPattern);

        if (searchQuery_.isEmpty() && searchInHistory_)
        {
            emit Utils::InterConnector::instance().hideSearchSpinner();

            clear_match();

            if (isSearchInDialog_)
                emitChanged(0, count());
            return;
        }

        if (getTypingTimeout())
            timerSearch_->start();
        else
            search();
    }

    void SearchModelDLG::search()
    {
       const auto haveMessagesPrevSearch = ((searchInHistory_ || isSearchInDialog_) && (LastRequestId_ == -1 || (LastRequestId_ != -1 && messagesCount_ != 0)));
       const auto haveContactsPrevSearch = !isSearchInDialog_ && (LastRequestId_ == -1 || (LastRequestId_ != -1 && contactsCount_ != 0));

       const auto prevQueInNew =
           !searchQuery_.isEmpty() &&
           !prevSearchQuery_.isEmpty() &&
           searchQuery_.startsWith(prevSearchQuery_) &&
           searchQuery_.length() >= prevSearchQuery_.length() &&
           !searchQuery_.endsWith(QChar::Space);

       const auto prevLocalSearchEmpty = prevQueInNew && !haveContactsPrevSearch && !haveMessagesPrevSearch;

        clear_match();
        emitChanged(0, count());

        if (!prevLocalSearchEmpty)
        {
            qDebug() << "doing local search for" << searchQuery_;
            searchLocal();
        }

        if (isGlobalSearchNeeded())
            searchGlobalContacts(searchQuery_.toStdString(), std::string(), std::string());

        emit Utils::InterConnector::instance().hideNoContactsYet();
        emit Utils::InterConnector::instance().hideNoSearchResults();
        emit Utils::InterConnector::instance().showSearchSpinner(this);
        emit Utils::InterConnector::instance().resetSearchResults();
    }

    void SearchModelDLG::globalResultsTimeout()
    {
        onRequestReturned();
    }

    void SearchModelDLG::messageResultsTimeout()
    {
        emit dataChanged(index(contactsCount_), index(count()));
        emit results();
    }

    void SearchModelDLG::searchEnded()
    {
        Ui::GetDispatcher()->post_message_to_core(qsl("history_search_ended"), nullptr);
        searchQuery_.clear();
        prevSearchQuery_.clear();
        LastRequestId_ = -1;
    }

    void SearchModelDLG::searchedMessage(Data::DlgState dlgState)
    {
        if (LastRequestId_ != dlgState.RequestId_ || currentViewMode_ != searchModel::searchView::results_page)
            return;

        qDebug() << __FUNCTION__ << "message" << dlgState.GetText();

        if (topMessageKeys_.size() < common::get_limit_search_results())
        {
            Match_.emplace_back(dlgState);
            topMessageKeys_.insert(std::make_pair(dlgState.SearchedMsgId_, Match_.size() - 1));

            onRequestReturned();
            messagesCount_++;
        }
        else
        {
            auto greater = topMessageKeys_.upper_bound(dlgState.SearchedMsgId_);
            if (greater == topMessageKeys_.end())
                return;

            auto index = topMessageKeys_.rbegin()->second;
            topMessageKeys_.erase(topMessageKeys_.rbegin()->first);
            topMessageKeys_.insert(std::make_pair(dlgState.SearchedMsgId_, index));
            Match_[index] = dlgState;
        }

        timerSortedResultsShow_->start();
        timerSort_->start();
    }

    void SearchModelDLG::searchedContacts(const QVector<Data::DlgState>& contacts, qint64 reqId)
    {
        qDebug() << __FUNCTION__ << "returned" << contacts.size() << "items, reqId" << reqId;
        if (LastRequestId_ != reqId || isSearchInDialog_)
        {
            qDebug() << __FUNCTION__ << "LastRequestId_" << LastRequestId_ << "reqId" << reqId;
            return;
        }

        for (const auto& c : contacts)
        {
            if (ContactsOnly_ && c.Chat_)
                continue;

            Match_.emplace_back(c);
            ++contactsCount_;
        }

        sortDialogs();

        onRequestReturned();

        if (contactsCount_ > 0)
        {
            emit Utils::InterConnector::instance().hideNoSearchResults();
            emit results();
        }
        else
            emit emptyContactResults();
    }

    void SearchModelDLG::searchGlobalContacts(const std::string& _keyword, const std::string& _phoneNumber, const std::string& _tag)
    {
        lastGlobalKeyword_ = _keyword;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("keyword", _keyword);
        collection.set_value_as_string("phonenumber", _phoneNumber);
        collection.set_value_as_string("tag", _tag);

        qDebug() << "requested results for " << _keyword.c_str();

        Ui::GetDispatcher()->post_message_to_core(qsl("contacts/search"), collection.get(), this, [this, _keyword](core::icollection* _coll)
        {
            if (lastGlobalKeyword_ != _keyword)
            {
                qDebug() << "denied results for" << _keyword.c_str() << "current keyword=" << lastGlobalKeyword_.c_str();
                return;
            }
            timerGlobalResults_->stop();

            Ui::gui_coll_helper coll(_coll, false);
            onGlobalContactSearchResult(coll, _keyword);
        });
        timerGlobalResults_->start();
    }

    void SearchModelDLG::onGlobalContactSearchResult(Ui::gui_coll_helper _coll, const std::string& _keyword)
    {
        __INFO("global_search", "received results for " << _keyword.c_str());

        bool dataAdded = false;
        const auto searchTerm = _keyword.c_str();
        if (_coll.is_value_exist("data"))
        {
            core::ifptr<core::iarray> cont_array(_coll.get_value_as_array("data"), false);

            __INFO("global_search", "contacts count=" << cont_array->size());

            for (int32_t i = 0; i < cont_array->size(); ++i)
            {
                Ui::gui_coll_helper coll_profile(cont_array->get_at(i)->get_as_collection(), false);

                Logic::contact_profile profile;
                if (profile.unserialize2(coll_profile))
                {
                    if (Logic::getContactListModel()->contains(profile.get_aimid()))
                        continue;

                    Data::DlgState c;
                    c.Friendly_ = profile.get_contact_name();
                    c.IsContact_ = true;
                    c.Chat_ = false;
                    c.AimId_ = profile.get_aimid();
                    c.IsFromSearch_ = true;
                    c.isFromGlobalSearch_ = true;
                    c.SearchTerm_ = searchTerm;

                    const auto mutualCount = profile.get_mutual_friends_count();
                    if (mutualCount)
                    {
                        c.SetText(QString::number(mutualCount) % ql1c(' ') %
                            Utils::GetTranslator()->getNumberString(
                                mutualCount,
                                QT_TRANSLATE_NOOP3("contact_list", "mutual friend", "1"),
                                QT_TRANSLATE_NOOP3("contact_list", "mutual friends", "2"),
                                QT_TRANSLATE_NOOP3("contact_list", "mutual friends", "5"),
                                QT_TRANSLATE_NOOP3("contact_list", "mutual friends", "21")
                            ));
                    }

                    globalContacts_.emplace_back(c);
                    if (globalContacts_.size() <= maxGlobalPreviewCount)
                    {
                        previewGlobalContactsCount_++;

                        auto it = std::find_if_not(Match_.begin(), Match_.end(), [](const auto& dlg) { return dlg.IsContact_; });
                        Match_.emplace(it, c);

                        dataAdded = true;
                    }
                }
            }
        }

        if (_coll->is_value_exist("chats"))
        {
            core::ifptr<core::iarray> chat_array(_coll.get_value_as_array("chats"), false);

            __INFO("global_search", "chats count=" << chat_array->size());

            for (int32_t i = 0; i < chat_array->size(); ++i)
            {
                Ui::gui_coll_helper coll_chat(chat_array->get_at(i)->get_as_collection(), false);

                Data::ChatInfo chat = Data::UnserializeChatInfo(&coll_chat);

                if (Logic::getContactListModel()->contains(chat.AimId_))
                    continue;

                Data::DlgState c;
                c.Friendly_ = chat.Name_;
                c.IsContact_ = true;
                c.Chat_ = true;
                c.AimId_ = chat.AimId_;
                c.IsFromSearch_ = true;
                c.isFromGlobalSearch_ = true;
                c.SearchTerm_ = searchTerm;

                c.SetText(QString::number(chat.MembersCount_) % ql1c(' ') %
                    Utils::GetTranslator()->getNumberString(
                        chat.MembersCount_,
                        QT_TRANSLATE_NOOP3("groupchats", "member", "1"),
                        QT_TRANSLATE_NOOP3("groupchats", "members", "2"),
                        QT_TRANSLATE_NOOP3("groupchats", "members", "5"),
                        QT_TRANSLATE_NOOP3("groupchats", "members", "21")
                    ));

                globalChats_.emplace_back(c);
                globalChatsData_.emplace_back(chat);
                dataAdded = true;
            }
        }

        if (dataAdded)
        {
            emit Utils::InterConnector::instance().hideSearchSpinner();
            emit Utils::InterConnector::instance().hideNoSearchResults();
            emitChanged(0, count());
            timerSort_->start();
        }
        onRequestReturned();
    }

    void SearchModelDLG::searchLocal()
    {
        SearchPatterns_ = Utils::GetPossibleStrings(searchQuery_, searchPatternsCount_);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        if (!SearchPatterns_.empty())
        {
            core::ifptr<core::iarray> symbolsArray(collection->create_array());
            symbolsArray->reserve(SearchPatterns_.size());

            for (auto& patternList : SearchPatterns_)
            {
                core::coll_helper symbol_coll(collection->create_collection(), true);
                core::ifptr<core::iarray> patternForSymbolArray(symbol_coll->create_array());
                patternForSymbolArray->reserve(patternList.size());

                for (auto& pattern : patternList)
                {
                    core::coll_helper coll(symbol_coll->create_collection(), true);

                    const auto pat = pattern.toUtf8();
                    coll.set_value_as_string("symbol_pattern", pat.data(), pat.size());
                    core::ifptr<core::ivalue> val(collection->create_value());
                    val->set_as_collection(coll.get());
                    patternForSymbolArray->push_back(val.get());
                }
                symbol_coll.set_value_as_array("symbols_patterns", patternForSymbolArray.get());
                core::ifptr<core::ivalue> symbols_val(symbol_coll->create_value());
                symbols_val->set_as_collection(symbol_coll.get());
                symbolsArray->push_back(symbols_val.get());
            }
            collection.set_value_as_array("symbols_array", symbolsArray.get());
            collection.set_value_as_uint("fixed_patterns_count", searchPatternsCount_);
        }
        else
        {
            collection.set_value_as_uint("fixed_patterns_count", 1);
        }

        if (isSearchInDialog_)
        {
            assert(!aimid_.isEmpty());
            collection.set_value_as_qstring("aimid", aimid_);
        }

        const auto pattern = searchQuery_.toUtf8();
        collection.set_value_as_string("init_pattern", pattern.data(), pattern.size());
        collection.set_value_as_bool("search_in_history", searchInHistory_);
        LastRequestId_ = Ui::GetDispatcher()->post_message_to_core(qsl("history_search"), collection.get());
        prevSearchQuery_ = searchQuery_;
    }

    void SearchModelDLG::onRequestReturned()
    {
        requestsReturnedCount_++;

        const auto messages = ((searchInHistory_ || isSearchInDialog_) && LastRequestId_ != -1)? 1 : 0;
        const auto contacts = (!isSearchInDialog_ && LastRequestId_ != -1) ? 1 : 0;
        const auto global = isGlobalSearchNeeded() ? 1 : 0;

        const auto maxReturns = messages + contacts + global;
        if (requestsReturnedCount_ >= maxReturns && Match_.empty())
        {
            emit Utils::InterConnector::instance().hideSearchSpinner();
            emit Utils::InterConnector::instance().showNoSearchResults();
        }
    }

    bool SearchModelDLG::isGlobalSearchNeeded() const
    {
        return searchInHistory_ && !isSearchInDialog_ && searchQuery_.length() >= 3;
    }

    void SearchModelDLG::avatarLoaded(const QString& aimid)
    {
        int i = 0;
        for (const auto& iter : Match_)
        {
            if (iter.AimId_ == aimid)
            {
                emitChanged(i, i);
                break;
            }
            ++i;
        }
    }

    void SearchModelDLG::contactRemoved(QString contact)
    {
        if (!Match_.empty())
        {
            const auto it = std::find_if(
                Match_.begin(),
                Match_.end(),
                [&contact](const Data::DlgState& item) { return item.AimId_ == contact && item.IsContact_; });

            if (it != Match_.end())
            {
                Match_.erase(it);
                --contactsCount_;

                const auto newCount = count();
                emitChanged(0, newCount);
                if (newCount == 0)
                    emit Utils::InterConnector::instance().showNoSearchResults();
            }
        }
    }

	void SearchModelDLG::emitChanged(int first, int last)
	{
		emit dataChanged(index(first), index(last));
	}

    void SearchModelDLG::sortDialogs()
	{
        {
            QSignalBlocker sb(this);

            std::sort(Match_.begin(), Match_.end(), [this](const Data::DlgState& _first, const Data::DlgState& _second)
            {
                if (_first.IsContact_ != _second.IsContact_)
                    return _first.IsContact_;

                if (_first.IsContact_)
                {
                    if (_first.isFromGlobalSearch_ != _second.isFromGlobalSearch_)
                        return _second.isFromGlobalSearch_;
                    else if (_first.isFromGlobalSearch_ && _first.Chat_ != _second.Chat_)
                        return _second.Chat_;

                    if (_first.SearchPriority_ == _second.SearchPriority_)
                    {
                        const auto fc = Logic::getContactListModel()->getContactItem(_first.AimId_);
                        if (!fc)
                            return false;

                        const auto sc = Logic::getContactListModel()->getContactItem(_second.AimId_);
                        if (!sc)
                            return true;

                        return _first.Time_ > _second.Time_;
                    }
                    return _first.SearchPriority_ < _second.SearchPriority_;
                }
                return _first.Time_ > _second.Time_;
            });

            topMessageKeys_.clear();
            int ind = 0;
            for (auto item : Match_)
            {
                if (!item.IsContact_)
                {
                    topMessageKeys_.emplace_hint(topMessageKeys_.end(), item.SearchedMsgId_, ind);
                }
                ++ind;
            }
        }

        emitChanged(0, count());
	}

    void SearchModelDLG::emptySearchResults(qint64 _seq)
    {
        qDebug() << __FUNCTION__;
        if (LastRequestId_ == _seq)
            onRequestReturned();
    }

    void SearchModelDLG::disableSearchInDialog()
    {
        setSearchInDialog(QString());
    }

    void SearchModelDLG::setSearchInDialog(const QString& _aimid)
    {
        const auto enabled = !_aimid.isEmpty();

        isSearchInDialog_ = enabled;
        aimid_ = _aimid;

        clear_match();

        if (enabled)
            searchEnded();
    }

    QString SearchModelDLG::getDialogAimid() const
    {
        return aimid_;
    }

    bool SearchModelDLG::isSearchInDialog() const
    {
        return isSearchInDialog_;
    }

    void SearchModelDLG::switchToResultsPage()
    {
        {
            QSignalBlocker sb(this);
            currentViewMode_ = searchModel::searchView::results_page;
            Match_ = tempMatch_;
        }
        emitChanged(0, count());
    }

    void SearchModelDLG::switchToAllPeoplePage()
    {
        {
            QSignalBlocker sb(this);

            currentViewMode_ = searchModel::searchView::global_contacts_page;

            tempMatch_ = Match_;
            Match_ = globalContacts_;
        }
        emitChanged(0, count());
        emit Utils::InterConnector::instance().globalSearchHeaderNeeded(QT_TRANSLATE_NOOP("contact_list", "All People"));
    }

    void SearchModelDLG::switchToAllChatsPage()
    {
        {
            QSignalBlocker sb(this);

            currentViewMode_ = searchModel::searchView::global_chats_page;

            tempMatch_ = Match_;
            Match_ = globalChats_;

        }
        emitChanged(0, count());
        emit Utils::InterConnector::instance().globalSearchHeaderNeeded(QT_TRANSLATE_NOOP("contact_list", "All Chats"));
    }

    void SearchModelDLG::repeatSearch()
    {
        searchPatternChanged(searchQuery_);
    }

    bool  SearchModelDLG::serviceItemClick(const QModelIndex & _index)
    {
        if (!_index.isValid())
            return false;

        if (_index.row() == getMorePeopleButtonIndex() && !globalContacts_.empty())
        {
            emit morePeopleClicked();
            return true;
        }
        else if (_index.row() == getMoreChatsButtonIndex() && !globalChats_.empty())
        {
            emit moreChatsClicked();
            return true;
        }
        return false;
    }

    searchModel::searchView SearchModelDLG::getCurrentViewMode() const
    {
        return currentViewMode_;
    }

    std::vector<Data::DlgState> SearchModelDLG::getMatch() const
    {
        return Match_;
    }

    void SearchModelDLG::setTypingTimeout(const int _timeout)
    {
        timerSearch_->setInterval(_timeout);
    }

    void SearchModelDLG::resetTypingTimeout()
    {
        setTypingTimeout(TYPING_TIMEOUT);
    }

    int SearchModelDLG::getTypingTimeout() const
    {
        return timerSearch_->interval();
    }

    SearchModelDLG* getSearchModelDLG()
    {
        static auto model = std::make_unique<SearchModelDLG>(nullptr);
        return model.get();
    }

    SearchModelDLG* getCustomSearchModelDLG(const bool _searchInHistory, const bool _contactsOnly)
    {
        static auto model = std::make_unique<SearchModelDLG>(nullptr);
        model->setSearchInHistory(_searchInHistory);
        model->setContactsOnly(_contactsOnly);
        return model.get();
    }
    SearchModelDLG* getSearchModelForMentions()
    {
        static auto model = std::make_unique<SearchModelDLG>(nullptr);
        model->setSearchInHistory(false);
        model->setContactsOnly(true);
        return model.get();
    }
}
