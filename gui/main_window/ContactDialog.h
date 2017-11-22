#pragma once

namespace Ui
{
    class HistoryControl;
    class HistoryControlPage;
    class InputWidget;
    class Sidebar;

    namespace Smiles
    {
        class SmilesMenu;
    }
    class ContactDialog;

    class DragOverlayWindow : public QWidget
    {
        Q_OBJECT

    public:
        DragOverlayWindow(ContactDialog* _parent);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void dragEnterEvent(QDragEnterEvent* _e) override;
        void dragLeaveEvent(QDragLeaveEvent* _e) override;
        void dragMoveEvent(QDragMoveEvent* _e) override;
        void dropEvent(QDropEvent* _e) override;

    private:
        ContactDialog* Parent_;
    };

    class ContactDialog : public QWidget
    {
        Q_OBJECT

    public Q_SLOTS:
        void onContactSelected(const QString& _aimId, qint64 _messageId, qint64 _quoteId);
        void onContactSelectedToLastMessage(QString _aimId, qint64 _messageId);

        void onSmilesMenu();
        void onInputEditFocusOut();
        void onSendMessage(const QString& _contact);

    private Q_SLOTS:
        void updateDragOverlay();
        void historyControlClicked();
        void onCtrlFPressedInInputWidget();
        void inputTyped();

    Q_SIGNALS:
        void contactSelected(const QString& _aimId, qint64 _messageId, qint64 _quoteId);
        void contactSelectedToLastMessage(QString _aimId, qint64 _messageId);

        void sendMessage(const QString&);
        void clicked();

    private:
        HistoryControl* historyControlWidget_;
        InputWidget* inputWidget_;
        Smiles::SmilesMenu* smilesMenu_;
        DragOverlayWindow* dragOverlayWindow_;
        Sidebar* sidebar_;
        QTimer* overlayUpdateTimer_;
        QStackedWidget* topWidget_;
        QVBoxLayout* rootLayout_;
        QMap<QString, QWidget*> topWidgetsCache_;

        void initSmilesMenu();
        void initInputWidget();

    public:
        ContactDialog(QWidget* _parent);
        ~ContactDialog();
        void cancelSelection();
        void hideInput();

        void showDragOverlay();
        void hideDragOverlay();

        void showSidebar(const QString& _aimId, int _page);
        void setSidebarVisible(bool _show);
        bool isSidebarVisible() const;

        void hideSmilesMenu();

        void insertTopWidget(const QString& _aimId, QWidget* _widget);
        void removeTopWidget(const QString& _aimId);

        HistoryControlPage* getHistoryPage(const QString& _aimId) const;
        const Smiles::SmilesMenu* getSmilesMenu() const;

        void setFocusOnInputWidget();
        Ui::InputWidget* getInputWidget() const;

        void notifyApplicationWindowActive(const bool isActive);

    protected:
        void dragEnterEvent(QDragEnterEvent *) override;
        void dragLeaveEvent(QDragLeaveEvent *) override;
        void dragMoveEvent(QDragMoveEvent *) override;
        void resizeEvent(QResizeEvent*) override;
    };
}