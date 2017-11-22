#pragma once

#include "../utils/InterConnector.h"

class QStandardItemModel;

namespace voip_manager
{
    struct Contact;
    struct ContactEx;
}

namespace Utils
{
    enum class CommonSettingsType;
    class SignalsDisconnector;
}

namespace Ui
{
    class main_page;
    class WidgetsNavigator;
    class ContactList;
    class SearchWidget;
    class CountrySearchCombobox;
    class VideoWindow;
    class VideoSettings;
    class IncomingCallWindow;
    class ContactDialog;
    class SearchContactsWidget;
    class GeneralSettingsWidget;
    class SelectContactsWidget;
    class HistoryControlPage;
    class ThemesSettingsWidget;
    class ContextMenu;
    class FlatMenu;
    class IntroduceYourself;
    class LabelEx;
    class LiveChatHome;
    class LiveChats;
    class TextEmojiWidget;
    class TopPanelWidget;
    class MainMenu;
    class SemitransparentWindowAnimated;
    class HorScrollableView;
    class CustomButton;
    class Sidebar;
    class SearchDropdown;

    namespace Stickers
    {
        class Store;
    }


    class UnreadsCounter : public QWidget
    {
        Q_OBJECT

    public:
        explicit UnreadsCounter(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* e) override;
    };

    class HeaderBack : public QPushButton
    {
        Q_OBJECT

    Q_SIGNALS:
        void resized();

    public:
        explicit HeaderBack(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* e) override;
        void resizeEvent(QResizeEvent* e) override;
    };

    class BackButton;

    enum class LeftPanelState
    {
        min,

        normal,
        picture_only,
        spreaded,

        max
    };

    class MainPage : public QWidget
    {
        Q_OBJECT

    public Q_SLOTS:
        void startSearhInDialog(QString _aimid);
        void setSearchFocus();
        void onAddContactClicked();
        void settingsClicked();

    private Q_SLOTS:
        void searchBegin();
        void searchEnd();
        void searchInputClear();
        void onContactSelected(QString _contact);
        void contactsClicked();
        void createGroupChat();
        void myProfileClicked();
        void aboutClicked();
        void contactUsClicked();
        // settings
        void onProfileSettingsShow(QString _uin);
        void onGeneralSettingsShow(int _type);
        void onThemesSettingsShow(bool, QString);
//        void onLiveChatsShow();
        //voip
        void onVoipShowVideoWindow(bool);
        void onVoipCallIncoming(const std::string&, const std::string&);
        void onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contacEx);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);

        void showPlaceholder(Utils::PlaceholdersType _placeholdersType);

        void post_stats_with_settings();
        void myInfo();
        void popPagesToRoot();
        void liveChatSelected();

        void spreadCL();
        void hideRecentsPopup();
        void searchActivityChanged(bool _isActive);

        void changeCLHeadToSearchSlot();
        void changeCLHeadToUnknownSlot();

        void openRecents();
        void showMainMenu();
        void compactModeChanged();
        void tabChanged(int);
        void themesSettingsOpen();
        void animFinished();
        void headerBack();
        void showHeader(const QString&);
        void currentPageChanged(int _index);

        void showStickersStore();

    private:
        explicit MainPage(QWidget* _parent);
        static MainPage* _instance;

    public:
        static MainPage* instance(QWidget* _parent = 0);
        static void reset();
        ~MainPage();
        void selectRecentChat(const QString& _aimId);
        void recentsTabActivate(bool _selectUnread = false);
        void settingsTabActivate(Utils::CommonSettingsType _item = Utils::CommonSettingsType::CommonSettingsType_None);
        void hideInput();
        void cancelSelection();
        void clearSearchMembers();
        void openCreatedGroupChat();

        void raiseVideoWindow();

        void nextChat();
        void prevChat();

        ContactDialog* getContactDialog() const;
        HistoryControlPage* getHistoryPage(const QString& _aimId) const;

        void insertTopWidget(const QString& _aimId, QWidget* _widget);
        void removeTopWidget(const QString& _aimId);

        void showSidebar(const QString& _aimId, int _page);
        bool isSidebarVisible() const;
        void setSidebarVisible(bool _show);
        void restoreSidebar();

        bool isContactDialog() const;

        static int getContactDialogWidth(int _mainPageWidth);

        Q_PROPERTY(int anim READ getAnim WRITE setAnim)

        void setAnim(int _val);
        int getAnim() const;

        Q_PROPERTY(int clWidth READ getCLWidth WRITE setCLWidth)

        void setCLWidth(int _val);
        int getCLWidth() const;

        void showVideoWindow();

        void notifyApplicationWindowActive(const bool isActive);
        bool isVideoWindowActive();

        void setFocusOnInput();
        void clearSearchFocus();

        void onSendMessage(const QString& contact);

        void hideMenu();
        bool isMenuVisible() const;
        bool isMenuVisibleOrOpening() const;

        void showSemiWindow();
        void hideSemiWindow();
        bool isSemiWindowVisible() const;

        static QString getMainWindowQss();

	void addButtonToTop(QWidget* _button);

    protected:
        void resizeEvent(QResizeEvent* _event) override;

    private:

        QWidget* showNoContactsYetSuggestions(QWidget* _parent);
        QWidget* showIntroduceYourselfSuggestions(QWidget* _parent);
        void animateVisibilityCL(int _newWidth, bool _withAnimation);
        void setLeftPanelState(LeftPanelState _newState, bool _withAnimation, bool _for_search = false, bool _force = false);
        void changeCLHead(bool _showUnknownHeader);

        void initAccessability();

    private:
        QWidget*                        unknownsHeader_;
        SearchDropdown*                 searchDropdown_;
        ContactList*                    contactListWidget_;
        SearchWidget*                   searchWidget_;
        VideoWindow*                    videoWindow_;
        VideoSettings*                  videoSettings_;
        WidgetsNavigator*               pages_;
        ContactDialog*                  contactDialog_;
        QVBoxLayout*                    pagesLayout_;

        SearchContactsWidget*           searchContacts_;
        GeneralSettingsWidget*          generalSettings_;
        ThemesSettingsWidget*           themesSettings_;
        Stickers::Store*                stickersStore_;

        LiveChatHome*                   liveChatsPage_;
        QHBoxLayout*                    horizontalLayout_;
        QWidget*                        noContactsYetSuggestions_;
        QWidget*                        introduceYourselfSuggestions_;
        bool                            needShowIntroduceYourself_;
        QTimer*                         settingsTimer_;
        bool                            recvMyInfo_;
        QPropertyAnimation*             animCLWidth_;
        QPropertyAnimation*             animBurger_;
        QWidget*                        clSpacer_;
        QVBoxLayout*                    contactsLayout;
        QHBoxLayout*                    originalLayout;
        QWidget*                        contactsWidget_;
        QHBoxLayout*                    clHostLayout_;
        LeftPanelState                  leftPanelState_;
        TopPanelWidget*                 myTopWidget_;
        MainMenu*                       mainMenu_;
        SemitransparentWindowAnimated*  semiWindow_;
        CustomButton*                   searchButton_;
        QWidget*                        headerWidget_;
        HeaderBack*                     headerBack_;
        UnreadsCounter*                 counter_;
        LabelEx*                        headerLabel_;
        Sidebar*                        embeddedSidebar_;
        bool                            NeedShowUnknownsHeader_;
        bool                            menuVisible_;
        int                             currentTab_;
        int                             anim_;

        std::map<std::string, std::shared_ptr<IncomingCallWindow> > incomingCallWindows_;
        std::unique_ptr<LiveChats> liveChats_;
        void destroyIncomingCallWindow(const std::string& _account, const std::string& _contact);
    };
}
