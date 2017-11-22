
namespace Ui {
    class MainWindow;
    class UnreadWidget;
}

class MacToolbar: public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void onUnreadMsgClicked();
    void onUnreadMailClicked();

public Q_SLOTS:
    void updateConnections();

private Q_SLOTS:
    void updateUnreadMailIcon(QString, unsigned, bool);
    void updateUnreadMsgIcon();

public:
    MacToolbar(Ui::MainWindow* _mainWindow);
    virtual ~MacToolbar();

    void setup();
    void setTitleText(QString _text);
    void onToolbarItemClicked(void* _itemPtr);
    void setUnreadMsgWidget(Ui::UnreadWidget* _widget);
    void setUnreadMailWidget(Ui::UnreadWidget* _widget);
    void setTitleIconsVisible(bool _unreadMsgVisible, bool _unreadMailVisible);

private:
    void initWindowTitle();
    void forceUpdateIcon(Ui::UnreadWidget* _widget, unsigned _iconTag, unsigned _count);

private:
    QString email_;
    Ui::MainWindow* mainWindow_;
    Ui::UnreadWidget* unreadMsgWdg_;
    Ui::UnreadWidget* unreadMailWdg_;
    unsigned unreadMsgCounter_;
    unsigned unreadMailCounter_;
};
