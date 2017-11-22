#pragma once

namespace Ui
{
    class ActionButton;
    class SearchWidget;
    class CustomButton;

    class SearchDropdown : public QWidget
    {
        Q_OBJECT

        enum class pencilIcon
        {
            pi_pencil,
            pi_cross
        };

    public Q_SLOTS:
        void showPlaceholder();
        void showMenuFull();
        void showMenuAddContactOnly();

    private Q_SLOTS:
        void createChat();
        void addContact();
        void onSearchActiveChanged(bool _isActive);
        void onPencilClicked();

    public:
        SearchDropdown(QWidget* _parent, SearchWidget* _searchWidget, CustomButton* _pencil);

        void updatePencilIcon(const pencilIcon _pencilIcon);

    protected:
        virtual void hideEvent(QHideEvent *_event) override;

    private:
        SearchWidget*   searchWidget_;
        CustomButton*   searchPencil_;

        QWidget*        menuWidget_;
        ActionButton*   addContact_;
        ActionButton*   createGroupChat_;
        ActionButton*   createChannel_;
        ActionButton*   createLiveChat_;

        QWidget*        placeholderUseSearch_;
    };
}
