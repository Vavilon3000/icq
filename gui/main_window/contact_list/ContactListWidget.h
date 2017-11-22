#pragma once
#include "ContactList.h"

namespace Logic
{
    class SearchModelDLG;
    enum class UpdateChatSelection;
}

namespace Ui
{
    class FocusableListView;
    class SearchWidget;
    class ContextMenu;

    class SearchInAllChatsButton : public QWidget
    {
        Q_OBJECT
            Q_SIGNALS :
        void clicked();
    public:
        explicit SearchInAllChatsButton(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent*) override;
        void enterEvent(QEvent *) override;
        void leaveEvent(QEvent *) override;
        void mousePressEvent(QMouseEvent *) override;
        void mouseReleaseEvent(QMouseEvent *) override;

    private:
        bool hover_;
        bool select_;
    };

    class SearchInChatLabel : public QWidget
    {
        Q_OBJECT

    public:
        SearchInChatLabel(QWidget* _parent, Logic::AbstractSearchModel* _searchModel);

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        Logic::AbstractSearchModel* model_;
    };

    class EmptyIgnoreListLabel : public QWidget
    {
        Q_OBJECT
    public:
        explicit EmptyIgnoreListLabel(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent*) override;
    };

    class GlobalSearchHeader : public QWidget
    {
        Q_OBJECT

    public:
        explicit GlobalSearchHeader(QWidget* _parent = nullptr);
        void setCaption(const QString& _caption);

    protected:
        void paintEvent(QPaintEvent*) override;
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void mouseMoveEvent(QMouseEvent*) override;
        void leaveEvent(QEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;

    private:
        void updateBackHoverState(const bool _hovered);

        QString caption_;
        QPixmap backButton_;
        int backWidth_;
        bool backHovered_;
    };

    class ContactListWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void searchEnd();
        void itemSelected(const QString&, qint64, qint64);
        void itemClicked(const QString&);
        void groupClicked(int);
        void changeSelected(const QString&);

    public:
        ContactListWidget(QWidget* _parent, const Logic::MembersWidgetRegim& _regim, Logic::ChatMembersModel* _chatMembersModel, Logic::AbstractSearchModel* _searchModel = nullptr);
        ~ContactListWidget();

        void connectSearchWidget(SearchWidget* _widget);

        void installEventFilterToView(QObject* _filter);
        void setIndexWidget(int index, QWidget* widget);

        void setClDelegate(Logic::AbstractItemDelegateWithRegim* _delegate);
        void setWidthForDelegate(int _width);
        void setDragIndexForDelegate(const QModelIndex& _index);

        void setEmptyIgnoreLabelVisible(bool _isVisible);
        void setSearchInDialog(bool _isSearchInDialog, bool _switchModel = true);
        bool getSearchInDialog() const;

        bool isSearchMode() const;
        QString getSelectedAimid() const;
        Logic::MembersWidgetRegim getRegim() const;

        Logic::AbstractSearchModel* getSearchModel() const;
        FocusableListView* getView() const;

        void triggerTapAndHold(bool _value);
        bool tapAndHoldModifier() const;

    public Q_SLOTS:
        void searchResult();
        void searchUpPressed();
        void searchDownPressed();
        void selectionChanged(const QModelIndex &);
        void select(const QString&);
        void select(const QString&, const qint64 _message_id, const qint64 _quote_id, Logic::UpdateChatSelection _mode);

    private Q_SLOTS:
        void itemClicked(const QModelIndex&);
        void itemPressed(const QModelIndex&);
        void onSearchResults();
        void searchResults(const QModelIndex &, const QModelIndex &);
        void searchClicked(const QModelIndex& _current);
        void showPopupMenu(QAction* _action);
        void showContactsPopupMenu(const QString& aimId, bool _is_chat);

        void searchPatternChanged(const QString&);
        void onDisableSearchInDialogButton();
        void touchScrollStateChanged(QScroller::State _state);

        void showNoContactsYet();
        void hideNoContactsYet();
        void showNoSearchResults();
        void hideNoSearchResults();
        void showSearchSpinner(Logic::SearchModelDLG* _senderModel);
        void hideSearchSpinner();

        void setGlobalSearchHeader(const QString & _caption);
        void hideGlobalSearchHeader();
        void onGlobalHeaderBackClicked();

        void scrollRangeChanged(int _min, int _max);

    private:
        void switchToInitial(bool _initial);
        void searchUpOrDownPressed(bool _isUp);
        void initSearchModel(Logic::AbstractSearchModel* _searchModel);
        bool isSelectMembersRegim() const;

    private:
        QVBoxLayout*									layout_;
        FocusableListView*								view_;

        EmptyIgnoreListLabel*							emptyIgnoreListLabel_;
        SearchInChatLabel*                              searchInChatLabel_;
        SearchInAllChatsButton*                         searchInAllButton_;
        GlobalSearchHeader*                             globalSearchHeader_;

        Logic::AbstractItemDelegateWithRegim*			clDelegate_;

        QWidget*										noContactsYet_;
        QWidget*										noSearchResults_;
        QWidget*										searchSpinner_;

        Logic::MembersWidgetRegim                       regim_;
        Logic::ChatMembersModel*                        chatMembersModel_;
        Logic::AbstractSearchModel*				        searchModel_;

        ContextMenu*                                    popupMenu_;

        bool											searchSpinnerShown_;
        bool                                            isSearchInDialog_;
        bool											noSearchResultsShown_;
        bool                                            noContactsYetShown_;
        bool                                            initial_;
        bool											tapAndHold_;

        int                                             scrollPosition_;
        QString                                         currentPattern_;
    };
}
