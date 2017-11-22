#pragma once

class QPushButton;

namespace Themes
{
    class IThemePixmap;

    typedef std::shared_ptr<IThemePixmap> IThemePixmapSptr;
}

namespace Emoji
{
    struct EmojiRecord;
    typedef std::shared_ptr<EmojiRecord> EmojiRecordSptr;
    typedef std::vector<EmojiRecordSptr> EmojiRecordSptrVec;
}

namespace Ui
{
    namespace stickers
    {
        class Set;
        class Sticker;
        typedef std::vector<std::shared_ptr<stickers::Set>> setsArray;
        typedef std::shared_ptr<Set> setSptr;
    }

    class smiles_Widget;

    namespace Smiles
    {
        class TabButton;
        class Toolbar;


        //////////////////////////////////////////////////////////////////////////
        // emoji_category
        //////////////////////////////////////////////////////////////////////////
        struct emoji_category
        {
            QString name_;
            const Emoji::EmojiRecordSptrVec& emojis_;

            emoji_category(const QString& _name, const Emoji::EmojiRecordSptrVec& _emojis)
                : name_(_name), emojis_(_emojis)
            {
            }
        };





        //////////////////////////////////////////////////////////////////////////
        // EmojiViewItemModel
        //////////////////////////////////////////////////////////////////////////
        class EmojiViewItemModel : public QStandardItemModel
        {
            QSize prevSize_;
            int emojisCount_;
            int needHeight_;
            bool singleLine_;
            int spacing_;

            std::vector<emoji_category> emojiCategories_;

            QVariant data(const QModelIndex& _idx, int _role) const;

        public:

            EmojiViewItemModel(QWidget* _parent, bool _singleLine = false);
            ~EmojiViewItemModel();

            int addCategory(const QString& _category);
            int addCategory(const emoji_category& _category);
            bool resize(const QSize& _size, bool _force = false);
            int getEmojisCount() const;
            int getNeedHeight() const;
            int getCategoryPos(int _index);
            const std::vector<emoji_category>& getCategories() const;
            void onEmojiAdded();
            int spacing() const;
            void setSpacing(int _spacing);

            Emoji::EmojiRecordSptr getEmoji(int _col, int _row) const;

        };




        //////////////////////////////////////////////////////////////////////////
        // EmojiTableItemDelegate
        //////////////////////////////////////////////////////////////////////////
        class EmojiTableItemDelegate : public QItemDelegate
        {
            Q_OBJECT

        public:
            EmojiTableItemDelegate(QObject* parent);

            void animate(const QModelIndex& index, int start, int end, int duration);

            void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
            QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

            Q_PROPERTY(int prop READ getProp WRITE setProp);

            int getProp() const { return Prop_; }
            void setProp(int val);

        private:
            int Prop_;
            QPropertyAnimation* Animation_;
            QModelIndex AnimateIndex_;
        };

        class EmojiTableView : public QTableView
        {
            EmojiViewItemModel* model_;
            EmojiTableItemDelegate* itemDelegate_;

        public:

            EmojiTableView(QWidget* _parent, EmojiViewItemModel* _model);
            ~EmojiTableView();

            void resizeEvent(QResizeEvent * _e) override;

            int addCategory(const QString& _category);
            int addCategory(const emoji_category& _category);
            int getCategoryPos(int _index);
            const std::vector<emoji_category>& getCategories() const;
            Emoji::EmojiRecordSptr getEmoji(int _col, int _row) const;
            void onEmojiAdded();

        };




        //////////////////////////////////////////////////////////////////////////
        // EmojisWidget
        //////////////////////////////////////////////////////////////////////////
        class EmojisWidget : public QWidget
        {
        Q_OBJECT

            EmojiTableView* view_;

            Toolbar* toolbar_;
            std::vector<TabButton*> buttons_;

            void placeToolbar(const QRect& _viewRect);

        Q_SIGNALS:

            void emojiSelected(Emoji::EmojiRecordSptr _emoji);
            void scrollToGroup(const int _pos);

        public:
            void onViewportChanged(const QRect& _rect, bool _blockToolbarSwitch);
            void selectFirstButton();
            EmojisWidget(QWidget* _parent);
            ~EmojisWidget();
        };




        class RecentsStickersTable;


        //////////////////////////////////////////////////////////////////////////
        // RecentsWidget
        //////////////////////////////////////////////////////////////////////////
        class RecentsWidget : public QWidget
        {
        Q_OBJECT
        Q_SIGNALS:
            void emojiSelected(Emoji::EmojiRecordSptr _emoji);
            void stickerSelected(qint32 _setId, qint32 _stickerId);
            void stickerHovered(qint32 _setId, qint32 _stickerId);
            void stickerPreview(qint32 _setId, qint32 _stickerId);
            void stickerPreviewClose();

        private Q_SLOTS:
            void stickers_event();

        protected:

            virtual void mouseReleaseEvent(QMouseEvent* _e) override;
            virtual void mousePressEvent(QMouseEvent* _e) override;
            virtual void mouseMoveEvent(QMouseEvent* _e) override;
            virtual void leaveEvent(QEvent* _e) override;

        private:
            Emoji::EmojiRecordSptrVec emojis_;

            QVBoxLayout* vLayout_;
            EmojiTableView* emojiView_;
            RecentsStickersTable* stickersView_;
            bool previewActive_;

            void init();
            void storeStickers();

        public:

            RecentsWidget(QWidget* _parent);
            ~RecentsWidget();

            void addSticker(int32_t _set_id, int32_t _stickerOd);
            void addEmoji(Emoji::EmojiRecordSptr _emoji);
            void initEmojisFromSettings();
            void initStickersFromSettings();
            void onStickerUpdated(int32_t _setOd, int32_t _stickerOd);
        };



        //////////////////////////////////////////////////////////////////////////
        // StickersTable
        //////////////////////////////////////////////////////////////////////////
        class StickersTable : public QWidget
        {
            Q_OBJECT
            Q_SIGNALS:

            void stickerSelected(qint32 _setId, qint32 _stickerId);
            void stickerPreview(qint32 _setId, qint32 _stickerId);
            void stickerHovered(qint32 _setId, qint32 _stickerId);

        private Q_SLOTS:

            void longtapTimeout();

        protected:

            QTimer* longtapTimer_;

            int32_t stickersSetId_;

            int needHeight_;

            QSize prevSize_;

            int columnCount_;

            int rowCount_;

            int32_t stickerSize_;

            int32_t itemSize_;

            std::pair<int32_t, int32_t> hoveredSticker_;

            Themes::IThemePixmapSptr preloader_;

            virtual void resizeEvent(QResizeEvent * _e) override;
            virtual void paintEvent(QPaintEvent* _e) override;

            QRect& getStickerRect(int _index) const;

            virtual bool resize(const QSize& _size, bool _force = false);

            virtual std::pair<int32_t, int32_t> getStickerFromPos(const QPoint& _pos) const;

            virtual void drawSticker(QPainter& _painter, const int32_t _setId, const int32_t _stickerId, const QRect& _rect);

            virtual void redrawSticker(const int32_t _setId, const int32_t _stickerId);

            int getNeedHeight() const;

        public:

            virtual void mouseReleaseEvent(const QPoint& _pos);
            virtual void mousePressEvent(const QPoint& _pos);
            virtual void mouseMoveEvent(const QPoint& _pos);
            virtual void leaveEvent();

            virtual void onStickerUpdated(int32_t _setId, int32_t _stickerId);
            void onStickerAdded();

            StickersTable(
                QWidget* _parent,
                const int32_t _stickersSetId,
                const qint32 _stickerSize,
                const qint32 _itemSize);

            virtual ~StickersTable();
        };

        typedef std::vector<std::pair<int32_t, int32_t>> recentStickersArray;





        //////////////////////////////////////////////////////////////////////////
        // RecentsStickersTable
        //////////////////////////////////////////////////////////////////////////
        class RecentsStickersTable : public StickersTable
        {
            recentStickersArray recentStickersArray_;

            int maxRowCount_;

            virtual bool resize(const QSize& _size, bool _force = false) override;
            virtual void paintEvent(QPaintEvent* _e) override;

            virtual std::pair<int32_t, int32_t> getStickerFromPos(const QPoint& _pos) const override;
            virtual void redrawSticker(const int32_t _setId, const int32_t _stickerId) override;
        public:

            void clear();

            bool addSticker(int32_t _setId, int32_t _stickerId);
            virtual void onStickerUpdated(int32_t _setId, int32_t _stickerId) override;

            void setMaxRowCount(int _val);

            const recentStickersArray& getStickers() const;

            RecentsStickersTable(QWidget* _parent, const qint32 _stickerSize, const qint32 _itemSize);
            virtual ~RecentsStickersTable();
        };





        //////////////////////////////////////////////////////////////////////////
        // StickersWidget
        //////////////////////////////////////////////////////////////////////////
        class StickersWidget : public QWidget
        {
            Q_OBJECT

        Q_SIGNALS:

            void stickerSelected(int32_t _setId, int32_t _stickerId);
            void stickerHovered(qint32 _setId, qint32 _stickerId);
            void stickerPreview(int32_t _setId, int32_t _stickerId);
            void stickerPreviewClose();
            void scrollToSet(int _pos);

        private Q_SLOTS:

        private:

            void insertNextSet(int32_t _setId);

            QVBoxLayout* vLayout_;

            Toolbar* toolbar_;

            std::map<int32_t, StickersTable*> setTables_;

            bool initialized_;

            bool previewActive_;

            virtual void mouseReleaseEvent(QMouseEvent* _e) override;
            virtual void mousePressEvent(QMouseEvent* _e) override;
            virtual void mouseMoveEvent(QMouseEvent* _e) override;
            virtual void leaveEvent(QEvent* _e) override;

        public:

            void init();
            void clear();
            void onStickerUpdated(int32_t _setId, int32_t _stickerId);

            StickersWidget(QWidget* _parent, Toolbar* _toolbar);
            ~StickersWidget();
        };







        //////////////////////////////////////////////////////////////////////////
        // StickerPreview
        //////////////////////////////////////////////////////////////////////////
        class StickerPreview : public QWidget
        {
            Q_OBJECT

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;

        public:

            StickerPreview(
                QWidget* _parent,
                const int32_t _setId,
                const int32_t _stickerId);

            void showSticker(
                const int32_t _setId,
                const int32_t _stickerId);

        private:
            int32_t setId_;
            int32_t stickerId_;

            QPixmap sticker_;
        };







        //////////////////////////////////////////////////////////////////////////
        // SmilesMenu
        //////////////////////////////////////////////////////////////////////////
        class SmilesMenu : public QFrame
        {
            Q_OBJECT
            Q_SIGNALS:

            void emojiSelected(int32_t _main, int32_t _ext);
            void stickerSelected(int32_t _setId, int32_t _stickerId);
            void menuHidden();

        private Q_SLOTS:

                void im_created();
                void touchScrollStateChanged(QScroller::State);
                void stickersMetaEvent();
                void stickerEvent(const qint32 _error, const qint32 _setId, const qint32 _stickerId);

        public:

            SmilesMenu(QWidget* _parent);
            ~SmilesMenu();

            void Show();
            void Hide();
            void ShowHide();
            bool IsHidden() const;

            Q_PROPERTY(int currentHeight READ getCurrentHeight WRITE setCurrentHeight)

            void setCurrentHeight(int _val);
            int getCurrentHeight() const;

        private:

            Toolbar* topToolbar_;
            Toolbar* bottomToolbar_;

            QScrollArea* viewArea_;

            RecentsWidget* recentsView_;
            EmojisWidget* emojiView_;
            StickersWidget* stickersView_;
            StickerPreview* stickerPreview_;
            QVBoxLayout* rootVerticalLayout_;
            QPropertyAnimation* animHeight_;
            QPropertyAnimation* animScroll_;

            bool isVisible_;
            bool blockToolbarSwitch_;

            bool stickerMetaRequested_;

            void InitSelector();
            void InitStickers();
            void InitResents();

            void ScrollTo(int _pos);
            void HookScroll();

            void showStickerPreview(const int32_t _setId, const int32_t _stickerId);
            void updateStickerPreview(const int32_t _setId, const int32_t _stickerId);
            void hideStickerPreview();

        protected:

            int currentHeight_;

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void focusOutEvent(QFocusEvent* _event) override;
            virtual void focusInEvent(QFocusEvent* _event) override;
            virtual void resizeEvent(QResizeEvent * _e) override;

        };
    }
}