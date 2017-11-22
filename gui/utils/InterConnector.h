#pragma once

namespace Ui
{
    class MainWindow;
    class HistoryControlPage;
    class ContactDialog;
}


namespace Data
{
    class ChatInfo;
    class DlgState;
}

namespace Logic
{
    enum class scroll_mode_type;
    class SearchModelDLG;
}

namespace Utils
{
    enum class CommonSettingsType
    {
        min = -1,

        CommonSettingsType_None,
        CommonSettingsType_Profile,
        CommonSettingsType_General,
        CommonSettingsType_VoiceVideo,
        CommonSettingsType_Notifications,
        CommonSettingsType_Themes,
        CommonSettingsType_About,
        CommonSettingsType_ContactUs,
        CommonSettingsType_AttachPhone,
        CommonSettingsType_AttachUin,

        max
    };

    enum class PlaceholdersType
    {
        min = -1,

        PlaceholdersType_FindFriend,
        PlaceholdersType_HideFindFriend,

        PlaceholdersType_IntroduceYourself,
        PlaceholdersType_SetExistanseOnIntroduceYourself,
        PlaceholdersType_SetExistanseOffIntroduceYourself,

        max
    };

    class InterConnector : public QObject
    {
        Q_OBJECT

Q_SIGNALS:
        void profileSettingsShow(const QString& uin);
        void profileSettingsBack();

        void themesSettingsOpen();

        void generalSettingsShow(int type);
        void generalSettingsBack();
        void themesSettingsShow(bool, const QString&);
        void themesSettingsBack();
        void profileSettingsUpdateInterface();

        void generalSettingsContactUsShown();

        void attachPhoneBack();
        void attachUinBack();

        void makeSearchWidgetVisible(bool);
        void showIconInTaskbar(bool _show);

        void popPagesToRoot();

        void showPlaceholder(PlaceholdersType);
        void showNoContactsYet();
        void hideNoContactsYet();

        void showNoRecentsYet();
        void hideNoRecentsYet();

        void showNoSearchResults();
        void hideNoSearchResults();

        void showSearchSpinner(Logic::SearchModelDLG* _senderModel);
        void hideSearchSpinner();
        void disableSearchInDialog();
        void repeatSearch();
        void dialogClosed(const QString& _aimid);

        void resetSearchResults();

        void onThemes();
        void cancelTheme(const QString&);
        void setToAllTheme(const QString&);
        void setTheme(const QString&);

        void closeAnyPopupWindow();
        void closeAnyPopupMenu();
        void closeAnySemitransparentWindow();

        void forceRefreshList(QAbstractItemModel *, bool);
        void updateFocus();
        void liveChatsShow();

        void schemeUrlClicked(const QString&);

        void setAvatar(qint64 _seq, int error);
        void setAvatarId(const QString&);

        void historyControlPageFocusIn(const QString&);

        void unknownsGoSeeThem();
        void unknownsGoBack();
        void unknownsDeleteThemAll();

        void liveChatSelected();
        void showLiveChat(std::shared_ptr<Data::ChatInfo> _info);

        void activateNextUnread();

        void clSortChanged();

        void historyControlReady(const QString&, qint64 _message_id, const Data::DlgState&, qint64 _last_read_msg, bool _isFirstRequest, Logic::scroll_mode_type _scrollMode);

        void imageCropDialogIsShown(QWidget *);
        void imageCropDialogIsHidden(QWidget *);
        void imageCropDialogMoved(QWidget *);
        void imageCropDialogResized(QWidget *);

        void startSearchInDialog(const QString&);
        void setSearchFocus();

        void searchEnd();
        void myProfileBack();

        void compactModeChanged();
        void mailBoxOpened();

        void logout();
        void authError(const int _error);
        void contacts();
        void showHeader(const QString&);

        void currentPageChanged();

        void showMessageHiddenControls();
        void hideMessageHiddenControls();

        void forceShowMessageTimestamps();
        void forceHideMessageTimestamps();
        void setTimestampHoverActionsEnabled(const bool _enabled);

        void globalSearchHeaderNeeded(const QString& _headerText);
        void hideGlobalSearchHeader();
        void globalHeaderBackClicked();

        void updateTitleButtons();
        void hideTitleButtons();
        void titleButtonsUpdated();

        void hideMentionCompleter();
        void showStickersStore();

        void addPageToDialogHistory(const QString& _aimid);
        void switchToPrevDialog();
        void clearDialogHistory();
        void pageAddedToDialogHistory();
        void noPagesInDialogHistory();

    public:
        static InterConnector& instance();
        ~InterConnector();

        void setMainWindow(Ui::MainWindow* window);
        Ui::MainWindow* getMainWindow() const;
        Ui::HistoryControlPage* getHistoryPage(const QString& aimId) const;
        Ui::ContactDialog* getContactDialog() const;

        void insertTopWidget(const QString& aimId, QWidget* widget);
        void removeTopWidget(const QString& aimId);

        void showSidebar(const QString& aimId, int page);
        void setSidebarVisible(bool show);
        bool isSidebarVisible() const;
        void restoreSidebar();

        void setDragOverlay(bool enable);
        bool isDragOverlay() const;

        void setFocusOnInput();
        void onSendMessage(const QString&);

    private:
        InterConnector();

        InterConnector(InterConnector&&);
        InterConnector(const InterConnector&);
        InterConnector& operator=(const InterConnector&);

        Ui::MainWindow* MainWindow_;
        bool dragOverlay_;
    };
}
