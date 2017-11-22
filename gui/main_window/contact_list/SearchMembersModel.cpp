#include "stdafx.h"
#include "SearchMembersModel.h"
#include "ChatMembersModel.h"
#include "ContactItem.h"
#include "AbstractSearchModel.h"
#include "ContactListModel.h"

#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"

namespace Logic
{
    SearchMembersModel::SearchMembersModel(QObject* _parent)
        : AbstractSearchModel(_parent)
        , searchRequested_(false)
        , selectEnabled_(true)
        , chatMembersModel_(nullptr)
    {
        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &SearchMembersModel::avatarLoaded, Qt::QueuedConnection);
    }

    int SearchMembersModel::rowCount(const QModelIndex &) const
    {
        return (int)match_.size();
    }

    QVariant SearchMembersModel::data(const QModelIndex & _ind, int _role) const
    {
        auto currentCount = match_.size();

        if (!_ind.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)) || (unsigned)_ind.row() >= currentCount)
            return QVariant();

        Data::ChatMemberInfo* ptr = &(match_[_ind.row()]);

        if (Testing::isAccessibleRole(_role))
            return match_[_ind.row()].AimId_;

        return QVariant::fromValue<Data::ChatMemberInfo*>(ptr);
    }

    Qt::ItemFlags SearchMembersModel::flags(const QModelIndex &) const
    {
        Qt::ItemFlags result = Qt::ItemIsEnabled;
        if (selectEnabled_)
            result |= Qt::ItemIsEnabled;

        return result;
    }

    void SearchMembersModel::setFocus()
    {
        match_.clear();
    }

    const QStringList& SearchMembersModel::getPattern() const
    {
        return searchPatterns_;
    }

    void SearchMembersModel::searchPatternChanged(QString _p)
    {
        match_.clear();

        lastSearchPattern_ = _p;

        if (!chatMembersModel_)
            return;

        auto match_whole = [](const auto& source, const auto& search)
        {
            if (int(source.size()) != int(search.size()))
                return false;

            auto source_iter = source.begin();
            auto search_iter = search.begin();

            while (source_iter != source.end() && search_iter != search.end())
            {
                if (!source_iter->startsWith(*search_iter, Qt::CaseInsensitive))
                    return false;

                ++source_iter;
                ++search_iter;
            }

            return true;
        };

        auto reverse_and_match_first_chars = [](auto& source, const auto& search)
        {
            if (int(source.size()) != int(search.size()))
                return false;

            std::reverse(source.begin(), source.end());

            auto source_iter = source.cbegin();
            auto search_iter = search.cbegin();

            while (source_iter != source.cend() && search_iter != search.cend())
            {
                if (search_iter->length() > 1 || !source_iter->startsWith(*search_iter, Qt::CaseInsensitive))
                    return false;

                ++source_iter;
                ++search_iter;
            }

            return true;
        };

        auto check_words = [match_whole, reverse_and_match_first_chars](auto pattern, auto item, auto displayName)
        {
            auto words = pattern.splitRef(QChar::Space, QString::SkipEmptyParts);

            if (words.size() > 1)
            {
                std::vector<QString> names = { item.FirstName_, item.LastName_ };

                if (match_whole(names, words) || reverse_and_match_first_chars(names, words))
                    return true;

                auto displayWords = displayName.splitRef(QChar::Space, QString::SkipEmptyParts);
                if (match_whole(displayWords, words) || reverse_and_match_first_chars(displayWords, words))
                    return true;

                auto nickWords = item.NickName_.splitRef(QChar::Space, QString::SkipEmptyParts);
                if (match_whole(nickWords, words) || reverse_and_match_first_chars(displayWords, words))
                    return true;
            }

            return false;
        };

        if (_p.isEmpty())
        {
            for (const auto& item : chatMembersModel_->members_)
            {
                match_.push_back(item);
            }
        }
        else
        {
            _p = std::move(_p).toLower();
            unsigned fixed_patterns_count = 0;
            auto searchPatterns = Utils::GetPossibleStrings(_p, fixed_patterns_count);
            for (const auto& item : chatMembersModel_->members_)
            {
                bool founded = false;
                const auto displayName = Logic::getContactListModel()->getDisplayName(item.AimId_).toLower();
                unsigned i = 0;
                for (; i < fixed_patterns_count; ++i)
                {
                    QString pattern;
                    if (searchPatterns.empty())
                    {
                        pattern = _p;
                    }
                    else
                    {
                        for (auto iter = searchPatterns.begin(); iter != searchPatterns.end(); ++iter)
                            pattern += iter->at(i);
                    }

                    if (item.AimId_.contains(pattern)
                        || displayName.contains(pattern)
                        || item.NickName_.contains(pattern, Qt::CaseInsensitive)
                        || item.FirstName_.contains(pattern, Qt::CaseInsensitive)
                        || item.LastName_.contains(pattern, Qt::CaseInsensitive)
                        || check_words(pattern, item, displayName))
                    {
                        match_.push_back(item);
                        founded = true;
                    }
                }

                int min = 0;
                for (const auto& s : searchPatterns)
                {
                    if (min == 0)
                        min = s.size();
                    else
                        min = std::min(min, s.size());
                }

                while (!searchPatterns.empty() && i < min && !founded)
                {
                    QString pattern;
                    for (const auto& iter : searchPatterns)
                    {
                        if (iter.size() > i)
                            pattern += iter.at(i);
                    }

                    pattern = std::move(pattern).toLower();

                    if (item.AimId_.contains(pattern)
                        || displayName.contains(pattern)
                        || item.NickName_.contains(pattern, Qt::CaseInsensitive)
                        || item.FirstName_.contains(pattern, Qt::CaseInsensitive)
                        || item.LastName_.contains(pattern, Qt::CaseInsensitive)
                        || check_words(pattern, item, displayName))
                    {
                        match_.push_back(item);
                        break;
                    }

                    ++i;
                }
            }
        }

        match_.erase(std::unique(match_.begin(), match_.end()), match_.end());
        int size = (int)match_.size();
        emit dataChanged(index(0), index(size));
    }

    void SearchMembersModel::searchResult(QStringList _result)
    {
        assert(!!"Methon is not implemented. Search in members don't use corelib.");
        return;
    }

    void SearchMembersModel::emitChanged(int _first, int _last)
    {
        emit dataChanged(index(_first), index(_last));
    }

    void SearchMembersModel::setChatMembersModel(ChatMembersModel* _chatMembersModel)
    {
        chatMembersModel_ = _chatMembersModel;
    }

    void SearchMembersModel::setSelectEnabled(bool _value)
    {
        selectEnabled_ = _value;
    }

    QString SearchMembersModel::getCurrentPattern() const
    {
        return lastSearchPattern_;
    }

    void SearchMembersModel::avatarLoaded(const QString& _aimId)
    {
        int i = 0;
        for (const auto& iter : match_)
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

    bool SearchMembersModel::isServiceItem(int i) const
    {
        return false;
    }

    SearchMembersModel* getSearchMemberModel(QObject* _parent)
    {
        return new SearchMembersModel(_parent);
    }
}
