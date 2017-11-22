#pragma once

#include "../../types/typing.h"
#include "../../controls/TransparentScrollBar.h"
#include "Common.h"
#include "ContactListUtils.h"

namespace Ui
{
    class CustomButton;
    class HorScrollableView;
    class ContactListWidget;
}

namespace Logic
{
    class UnknownItemDelegate;
	class RecentItemDelegate;
	class ContactListItemDelegate;
    class ChatMembersModel;
    class AbstractSearchModel;
    class LiveChatItemDelegate;
    enum class UpdateChatSelection;
}

namespace Data
{
	class Contact;
}

namespace Utils
{
    class SignalsDisconnector;
}

namespace Ui
{
    class SettingsTab;
    class ContextMenu;
    class ContactList;

    class RCLEventFilter : public QObject
    {
        Q_OBJECT

    public:
        RCLEventFilter(ContactList* _cl);
    protected:
        bool eventFilter(QObject* _obj, QEvent* _event);
    private:
        ContactList* cl_;
    };

	enum CurrentTab
	{
		RECENTS = 0,
        LIVE_CHATS,
		SETTINGS,
		SEARCH,
	};

	class ContactList : public QWidget
	{
		Q_OBJECT

	Q_SIGNALS:
		void itemSelected(const QString&, qint64 _message_id, qint64 _quote_id);
		void groupClicked(int);
        void needSwitchToRecents();
        void tabChanged(int);

	public Q_SLOTS:
        void onSendMessage(const QString&);
        void select(const QString&);
        void select(const QString&, qint64 _message_id, qint64 _quote_id, Logic::UpdateChatSelection _mode);

        void changeSelected(const QString& _aimId);
		void recentsClicked();
		void settingsClicked();
        void switchToRecents();

        void setSearchInDialog(bool _isSearchInDialog);
        bool getSearchInDialog() const;

	private Q_SLOTS:
		void itemClicked(const QModelIndex&);
		void itemPressed(const QModelIndex&);
        void liveChatsItemPressed(const QModelIndex&);
		void statsRecentItemPressed(const QModelIndex&);
		void statsCLItemPressed(const QModelIndex&);

        void guiSettingsChanged();
		void recentOrderChanged();
		void touchScrollStateChangedRecents(QScroller::State);
        void touchScrollStateChangedLC(QScroller::State);

        void showNoRecentsYet();
        void hideNoRecentsYet();

        void typingStatus(Logic::TypingFires _typing, bool _isTyping);

        void messagesReceived(const QString&, const QVector<QString>&);
		void showPopupMenu(QAction* _action);
        void autoScroll();

        void dialogClosed(const QString& _aimid);
        void myProfileBack();

        void recentsScrollActionTriggered(int value);

	public:

		ContactList(QWidget* _parent);
		~ContactList();

		void setSearchMode(bool);
		bool isSearchMode() const;
		void changeTab(CurrentTab _currTab, bool silent = false);
        inline CurrentTab currentTab() const
        {
            return (CurrentTab)currentTab_;
        }
        void triggerTapAndHold(bool _value);
        bool tapAndHoldModifier() const;
        void dragPositionUpdate(const QPoint& _pos, bool fromScroll = false);
        void dropFiles(const QPoint& _pos, const QList<QUrl> _files);
        void selectSettingsVoipTab();

        void setPictureOnlyView(bool _isPictureOnly);
        bool getPictureOnlyView() const;
        void setItemWidth(int _newWidth);
        void openThemeSettings();

        QString getSelectedAimid() const;

        SettingsTab* getSettingsTab() const { return settingsTab_; }
        ContactListWidget* getContactListWidget() const { return contactListWidget_; }

	private:
		void updateTabState();
		void showRecentsPopup_menu(const QModelIndex& _current);
		void showContactsPopupMenu(const QString& _aimid, bool _is_chat);

        void showNoRecentsYet(QWidget *_parent, QWidget *_list, QLayout *_layout, std::function<void()> _action);

    private:
        Logic::LiveChatItemDelegate*                    liveChatsDelegate_;
		RCLEventFilter*									listEventFilter_;
		QWidget*										liveChatsPage_;
		FocusableListView*								liveChatsView_;
        QWidget*										noRecentsYet_;
		Ui::ContextMenu*								popupMenu_;
		Logic::RecentItemDelegate*						recentsDelegate_;
        Logic::UnknownItemDelegate                      *unknownsDelegate_;

		QVBoxLayout*									recentsLayout_;
		QWidget*										recentsPage_;
		ListViewWithTrScrollBar*						recentsView_;

        ContactListWidget*                              contactListWidget_;

        SettingsTab*									settingsTab_;
		QStackedWidget*									stackedWidget_;
        QTimer*                                         scrollTimer_;
        FocusableListView*                              scrolledView_;
        int                                             scrollMultipler_;
        QPoint                                          lastDragPos_;

        unsigned										currentTab_;
        unsigned                                        prevTab_;
        bool											noRecentsYetShown_;
        bool											tapAndHold_;
        bool                                            pictureOnlyView_;
	};
}
