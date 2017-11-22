#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"

namespace Ui
{
    class MainWindow;
}

namespace Logic
{
	class RecentsModel : public CustomAbstractListModel
	{
		Q_OBJECT

	Q_SIGNALS:
		void orderChanged();
		void updated();
        void readStateChanged(const QString&);
        void selectContact(const QString&);
        void dlgStatesHandled(const QVector<Data::DlgState>&);
        void favoriteChanged(const QString&);

    public Q_SLOTS:
        void refresh();

	private Q_SLOTS:
		void activeDialogHide(const QString&);
		void contactChanged(const QString&);
		void dlgStates(const QVector<Data::DlgState>&);
		void sortDialogs();
        void contactRemoved(const QString&);

	public:
		explicit RecentsModel(QObject *parent);

		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		QVariant data(const QModelIndex &index, int role) const;
		Qt::ItemFlags flags(const QModelIndex &index) const;

		Data::DlgState getDlgState(const QString& aimId = QString(), bool fromDialog = false);
        void unknownToRecents(const Data::DlgState&);

        void toggleFavoritesVisible();

        void unknownAppearance();

		void sendLastRead(const QString& aimId = QString());
		void markAllRead();
		void hideChat(const QString& aimId);
		void muteChat(const QString& aimId, bool mute);

        bool isFavorite(const QString& aimid) const;
        bool isServiceItem(const QModelIndex& i) const;
        bool isFavoritesGroupButton(const QModelIndex& i) const;
        bool isFavoritesVisible() const;
        quint16 getFavoritesCount() const;
        bool isUnknownsButton(const QModelIndex& i) const;
        bool isRecentsHeader(const QModelIndex& i) const;
        void setFavoritesHeadVisible(bool _isVisible);

        bool isServiceAimId(const QString& _aimId) const;

		QModelIndex contactIndex(const QString& aimId) const;

		int totalUnreads() const;
        int recentsUnreads() const;
        int favoritesUnreads() const;

        QString firstContact() const;
        QString nextUnreadAimId() const;
        QString nextAimId(const QString& aimId) const;
        QString prevAimId(const QString& aimId) const;

        bool lessRecents(const QString& _aimid1, const QString& _aimid2);

        std::vector<QString> getSortedRecentsContacts() const;

	private:
        int correctIndex(int i) const;
        int visibleContactsInFavorites() const;

        int getUnknownsButtonIndex() const;
        int getSizeOfUnknownBlock() const;

        int getFavoritesHeaderIndex() const;
        int getRecentsHeaderIndex() const;
        int getVisibleServiceItemInFavorites() const;

		std::vector<Data::DlgState> Dialogs_;
		QHash<QString, int> Indexes_;
		QTimer* Timer_;
        quint16 FavoritesCount_;
        bool FavoritesVisible_;
        bool FavoritesHeadVisible_;
	};

	RecentsModel* getRecentsModel();
    void ResetRecentsModel();
}