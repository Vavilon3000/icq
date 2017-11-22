#pragma once

namespace Ui
{
    class ScrollAreaWithTrScrollBar;
    class TextEmojiWidget;

    struct PackInfo
    {
        int32_t id_;

        QString name_;
        QString comment_;
        QString storeId_;
        QPixmap icon_;

        PackInfo() : id_(-1) {}
        PackInfo(
            const int32_t _id,
            const QString& _name,
            const QString& _comment,
            const QString& _storeId,
            const QPixmap& _icon)
            : id_(_id)
            , name_(_name)
            , comment_(_comment)
            , storeId_(_storeId)
            , icon_(_icon)
        {
        }

        PackInfo(PackInfo&& _info) noexcept
            : id_(_info.id_)
            , name_(std::move(_info.name_))
            , comment_(std::move(_info.comment_))
            , storeId_(std::move(_info.storeId_))
            , icon_(std::move(_info.icon_))
        {
        }
    };

    namespace Stickers
    {
        //////////////////////////////////////////////////////////////////////////
        class PacksView : public QWidget
        {
            Q_OBJECT

            ScrollAreaWithTrScrollBar* parent_;

            std::vector<PackInfo> packs_;

            int hoveredPack_;

            QPropertyAnimation* animScroll_;

        private:

            enum direction
            {
                left = 0,
                right = 1
            };

            static QRect getStickerRect(const int _pos);
            void scrollStep(direction _direction);

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void wheelEvent(QWheelEvent* _e) override;
            virtual void mousePressEvent(QMouseEvent* _e) override;
            virtual void leaveEvent(QEvent* _e) override;
            virtual void mouseMoveEvent(QMouseEvent* _e) override;


        public:

            PacksView(ScrollAreaWithTrScrollBar* _parent);

            void addPack(PackInfo _pack);

            void updateSize();

            void onSetIcon(const int32_t _setId);

            void clear();
        };



        //////////////////////////////////////////////////////////////////////////
        class PacksWidget : public QWidget
        {
            Q_OBJECT

            PacksView* packs_;

            ScrollAreaWithTrScrollBar* scrollArea_;

        public:

            PacksWidget(QWidget* _parent);

            void init(const bool _fromServer);
            void onSetIcon(const int32_t _setId);
        };


        //////////////////////////////////////////////////////////////////////////
        class MyPacksView : public QWidget
        {
            Q_OBJECT

        private:

            std::vector<PackInfo> packs_;

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void mousePressEvent(QMouseEvent* _e) override;


            QRect getStickerRect(const int _pos);
            QRect getDelButtonRect(const QRect& _stickerRect);

        public:

            MyPacksView(QWidget* _parent);

            void addPack(PackInfo _pack);

            void updateSize();

            void clear();

            void onSetIcon(const int32_t _setId);

            bool empty() const;
        };

        class MyPacksHeader : public QWidget
        {
            Q_OBJECT
            Q_SIGNALS :

            void clicked(bool);

        private:

            QPushButton* buttonAdd_;
            const QFont font_;
            const QString createStickerPack_;
            const QFontMetrics metrics_;
            const int packLength_;
            const int marginText_;
            const int buttonWidth_;

        public:

            MyPacksHeader(QWidget* _parent);

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void resizeEvent(QResizeEvent * event) override;
        };


        //////////////////////////////////////////////////////////////////////////
        class MyPacksWidget : public QWidget
        {
            Q_OBJECT

            MyPacksView* packs_;

            bool syncedWithServer_;

            std::shared_ptr<bool> ref_;

        private Q_SLOTS:

            void createPackButtonClicked(bool);

        public:

            void onSetIcon(const int32_t _setId);

            MyPacksWidget(QWidget* _parent);

            void init(const bool _fromServer);

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;
        };


        //////////////////////////////////////////////////////////////////////////
        class Store : public QWidget
        {
            Q_OBJECT

            QScrollArea* rootScrollArea_;

            PacksWidget* packsView_;
            MyPacksWidget* myPacks_;

        private Q_SLOTS:

            void touchScrollStateChanged(QScroller::State _state);
            void onSetBigIcon(const qint32 _error, const qint32 _setId);
            void onStickers();
            void onStore();

        protected:

            virtual void resizeEvent(QResizeEvent * event) override;
            virtual void paintEvent(QPaintEvent* _e) override;

        public:

            Store(QWidget* _parent);


        };
    }
}
