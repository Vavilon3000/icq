#pragma once

#include "ContactItem.h"
#include "../../types/contact.h"
#include "../../types/message.h"
#include "../../types/chat.h"
#include "../../utils/gui_coll_helper.h"
#include "AbstractSearchModel.h"

namespace Logic
{
    namespace searchModel
    {
        enum class searchView
        {
            results_page,
            global_contacts_page,
            global_chats_page,
        };
    }

	class SearchModelDLG : public AbstractSearchModel
	{
		Q_OBJECT
	Q_SIGNALS:
        void emptyContactResults();
        void morePeopleClicked();
        void moreChatsClicked();

	public Q_SLOTS:
		void searchPatternChanged(QString) override;
        void searchedMessage(Data::DlgState);
        void searchedContacts(const QVector<Data::DlgState>&, qint64);
        void searchEnded();
        void disableSearchInDialog();
        void repeatSearch();

    private Q_SLOTS:
        void avatarLoaded(const QString&);
        void contactRemoved(QString contact);
        void sortDialogs();
        void emptySearchResults(qint64);

        void switchToResultsPage();
        void switchToAllPeoplePage();
        void switchToAllChatsPage();

        void search();
        void globalResultsTimeout();
        void messageResultsTimeout();

	public:
		explicit SearchModelDLG(QObject *parent);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role) const override;
		Qt::ItemFlags flags(const QModelIndex &index) const override;
		void setFocus() override;
		void emitChanged(int first, int last) override;
        virtual bool isServiceItem(int i) const override;
        virtual bool isServiceButton(const QModelIndex& _index) const;

        QString getDialogAimid() const;

        void setSearchInDialog(const QString& _aimid);
        bool isSearchInDialog() const;
        void clear_match();

        bool getChatInfo(const QModelIndex& _index, Out Data::ChatInfo& _info) const;

        void setSearchInHistory(const bool value);
        bool getSearchInHistory() const;

        void setContactsOnly(const bool value);
        bool getContactsOnly() const;

        virtual QString getCurrentPattern() const override;

        int count() const;
        int contactsCount() const;

        int getMessagesHeaderIndex() const;
        int getPeopleHeaderIndex() const;
        int getMorePeopleButtonIndex() const;
        int getMoreChatsButtonIndex() const;

        bool isResultFromGlobalSearch(const QModelIndex& _index) const;

        bool serviceItemClick(const QModelIndex& _index);

        searchModel::searchView getCurrentViewMode() const;

        std::vector<Data::DlgState> getMatch() const;

        void setTypingTimeout(const int _timeout);
        void resetTypingTimeout();
        int getTypingTimeout() const;

	private:
        int correctIndex(const int i) const;

        void searchGlobalContacts(const std::string& _keyword, const std::string& _phoneNumber, const std::string& _tag);
        void onGlobalContactSearchResult(Ui::gui_coll_helper _coll, const std::string& _keyword);

        void searchLocal();
        void onRequestReturned();

        bool isGlobalSearchNeeded() const;

        std::vector<Data::DlgState> Match_;
        std::vector<Data::DlgState> tempMatch_;

        std::vector<Data::DlgState> globalContacts_;
        std::vector<Data::DlgState> globalChats_;
        std::vector<Data::ChatInfo> globalChatsData_;

        std::map<int64_t, int, std::greater<int64_t>> topMessageKeys_;

        std::vector<QStringList> SearchPatterns_;
        uint32_t searchPatternsCount_;

        qint64 LastRequestId_;

        int32_t contactsCount_;
        int messagesCount_;
        int32_t previewGlobalContactsCount_;

        std::string lastGlobalKeyword_;

        QTimer* timerSort_;
        QTimer* timerSearch_;
        QTimer* timerGlobalResults_;
        QTimer* timerSortedResultsShow_;

        bool isSearchInDialog_;
        bool ContactsOnly_;
        bool searchInHistory_;

        QString aimid_;
        QString searchQuery_;
        QString prevSearchQuery_;

        int requestsReturnedCount_;

        searchModel::searchView currentViewMode_;
    };

    SearchModelDLG* getSearchModelDLG();
	SearchModelDLG* getCustomSearchModelDLG(const bool _searchInHistory, const bool _contactsObly);
    SearchModelDLG* getSearchModelForMentions();
}