#pragma once

#include "../../../namespaces.h"
#include "../../../types/message.h"

#include "../MessageItemBase.h"

UI_NS_BEGIN

class ActionButtonWidget;
class ContextMenu;
class HistoryControlPage;
class MessageTimeWidget;
class TextEmojiWidget;
class TextEditEx;

UI_NS_END

UI_THEMES_NS_BEGIN

class theme;

UI_THEMES_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType;
class ComplexMessageItemLayout;
class IItemBlock;

typedef std::shared_ptr<const QPixmap> QPixmapSCptr;

typedef std::vector<IItemBlock*> IItemBlocksVec;

class ComplexMessageItem final : public MessageItemBase
{
    friend class ComplexMessageItemLayout;

    enum class MenuItemType
    {
        Copy,
        Quote,
        Forward,
    };

    Q_OBJECT

Q_SIGNALS:
    void copy(QString text);

    void quote(const QVector<Data::Quote>&);

    void forward(const QVector<Data::Quote>&);

    void avatarMenuRequest(QString);

    void eventFilterRequest(QWidget*);

    void selectionChanged();
    void setTextEditEx(Ui::TextEditEx*);
    void leave();

public:
    ComplexMessageItem(
        QWidget *parent,
        const int64_t id,
        const QDate date,
        const QString &chatAimid,
        const QString &senderAimid,
        const QString &senderFriendly,
        const QString &sourceText,
        const Data::MentionMap& _mentions,
        const bool isOutgoing);

    virtual void clearSelection() override;

    virtual QString formatRecentsText() const override;

    qint64 getId() const override;

    QString getQuoteHeader() const;

    QString getSelectedText(const bool isQuote) const;

    const QString& getChatAimid() const;

    QDate getDate() const;

    const QString& getSenderAimid() const;

    QString getSenderFriendly() const;

    std::shared_ptr<const themes::theme> getTheme() const;

    virtual bool isOutgoing() const override;

    bool isSimple() const;

    bool isUpdateable() const;

    void onBlockSizeChanged();

    void onHoveredBlockChanged(IItemBlock *newHoveredBlock);

    void onStyleChanged();

    virtual void onActivityChanged(const bool isActive) override;

    virtual void onVisibilityChanged(const bool isVisible) override;

    virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    void replaceBlockWithSourceText(IItemBlock *block);

    void removeBlock(IItemBlock *block);

    void selectByPos(const QPoint& from, const QPoint& to);

    virtual void setHasAvatar(const bool value) override;

    void setItems(IItemBlocksVec blocks);

    void setMchatSender(const QString& sender);

    void setLastStatus(LastStatus _lastStatus) final override;

    void setTime(const int32_t time);

    int32_t getTime() const;

    virtual QSize sizeHint() const override;

    void updateWith(ComplexMessageItem &update);

    bool isSelected() const override;

    QVector<Data::Quote> getQuotes(bool force = false) const;

    void setSourceText(QString text);

    void forwardRoutine();

    virtual void setQuoteSelection() override;

    void setDeliveredToServer(const bool _isDeliveredToServer) override;

    bool isQuoteAnimation() const;

    /// observe to resize when replaced
    bool isObserveToSize() const;

    virtual int getMaxWidth() const;

    const Data::MentionMap& getMentions() const;

protected:

    virtual void leaveEvent(QEvent *event) override;

    virtual void mouseMoveEvent(QMouseEvent *event) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    virtual void paintEvent(QPaintEvent *event) override;
    virtual void hideEvent(QHideEvent*) override;

private Q_SLOTS:
    void onAvatarChanged(QString aimId);

    void onMenuItemTriggered(QAction *action);

    void contextMenuShow();

    void contextMenuHide();
    void showHiddenControls();
    void hideHiddenControls();
    void setTimestampHoverEnabled(const bool _enabled);

public Q_SLOTS:
    void trackMenu(const QPoint &globalPos);

    void setQuoteAnimation();
    void onObserveToSize();

private:

    void cleanupMenu();

    void createSenderControl();

    void connectSignals();

    bool containsShareableBlocks() const;

    void drawAvatar(QPainter &p);

    void drawBubble(QPainter &p, const QColor& quote_color);

    QString getBlocksText(const IItemBlocksVec &items, const bool isSelected, const bool isQuote) const;

    void drawGrid(QPainter &p);

    IItemBlock* findBlockUnder(const QPoint &pos) const;

    void initialize();

    void initializeShareButton();

    bool isBubbleRequired() const;

    bool isOverAvatar(const QPoint &pos) const;

    bool isSenderVisible() const;

    void loadAvatar();

    void onCopyMenuItem(MenuItemType type);

    bool onDeveloperMenuItemTriggered(const QString &cmd);

    void shareButtonRoutine(QString sourceText);
    void onShareButtonClicked();

    void updateSenderControlColor();

    void updateShareButtonGeometry();

    QPixmapSCptr Avatar_;

    IItemBlocksVec Blocks_;

    QPainterPath Bubble_;

    QRect BubbleGeometry_;

    QString ChatAimid_;

    QDate Date_;

    BlockSelectionType FullSelectionType_;

    IItemBlock *HoveredBlock_;

    int64_t Id_;

    bool Initialized_;

    bool IsOutgoing_;

    bool IsDeliveredToServer_;

    ComplexMessageItemLayout *Layout_;

    IItemBlock *MenuBlock_;

    bool MouseLeftPressedOverAvatar_;

    bool MouseRightPressedOverItem_;

    TextEmojiWidget *Sender_;

    QString SenderAimid_;

    QString SenderFriendly_;

    ActionButtonWidget *ShareButton_;

    QString SourceText_;

    MessageTimeWidget *TimeWidget_;

    int32_t Time_;

    bool bQuoteAnimation_;
    bool bObserveToSize_;
    bool timestampHoverEnabled_;
    bool bubbleHovered_;

    Data::MentionMap mentions_;
};

UI_COMPLEX_MESSAGE_NS_END
