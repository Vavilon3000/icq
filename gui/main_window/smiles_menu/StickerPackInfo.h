#pragma once

namespace Ui
{
    class CustomButton;
    class TextEditEx;

    namespace Stickers
    {
        class Set;
    };

    namespace Smiles
    {
        class StickersTable;
        class StickerPreview;
    }



    class GeneralDialog;


    class StickersView : public QWidget
    {
    Q_OBJECT

    Q_SIGNALS :

        void stickerPreviewClose();
        void stickerPreview(const int32_t _setId, const int32_t _stickerId);
        void stickerHovered(const int32_t _setId, const int32_t _stickerId);

    private:

        Smiles::StickersTable* stickers_;

        bool previewActive_;

    public:

        StickersView(QWidget* _parent, Smiles::StickersTable* _stickers);

        virtual void mouseReleaseEvent(QMouseEvent* _e) override;
        virtual void mousePressEvent(QMouseEvent* _e) override;
        virtual void mouseMoveEvent(QMouseEvent* _e) override;
        virtual void leaveEvent(QEvent* _e) override;
    };




    class PackWidget : public QWidget
    {
        Q_OBJECT

        QVBoxLayout* rootVerticalLayout_;

        QVBoxLayout* stickersLayout_;

        QScrollArea* viewArea_;

        Smiles::StickersTable* stickers_;

        Smiles::StickerPreview* stickerPreview_;

        QPushButton* addButton_;

        QPushButton* removeButton_;

        qint32 setId_;

        QString storeId_;

        QString description_;

        CustomButton* shareButton_;

        QLabel* loadingText_;

        TextEditEx* descriptionControl_;

        QWidget* dialog_;

    private Q_SLOTS:

        void onAddButton(bool _checked);
        void onRemoveButton(bool _checked);
        void onShareButton(bool _checked);
        void touchScrollStateChanged(QScroller::State _state);
        void onStickerPreviewClose();
        void onStickerPreview(const int32_t _setId, const int32_t _stickerId);
        void onStickerHovered(const int32_t _setId, const int32_t _stickerId);

    Q_SIGNALS:

        void buttonClicked();
        void shareClicked();

    public:

        PackWidget(QWidget* _parent);

        void onStickersPackInfo(std::shared_ptr<Ui::Stickers::Set> _set, const bool _result, const bool _purchased);
        void onStickerEvent(const qint32 _error, const qint32 _setId, const qint32 _stickerId);

        void setParentDialog(QWidget* _dialog);

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;
        virtual void resizeEvent(QResizeEvent* _e) override;

    private:

        void moveShareButton();
    };

    class StickerPackInfo : public QWidget
    {
        Q_OBJECT

        PackWidget* pack_;

        int32_t set_id_;

        QString store_id_;


    private Q_SLOTS:

        void onStickerpackInfo(const bool _result, std::shared_ptr<Ui::Stickers::Set> _set);
        void stickerEvent(const qint32 _error, const qint32 _setId, const qint32 _stickerId);
        void onShareClicked();

    public:

        StickerPackInfo(QWidget* _parent, const int32_t _set_id, const QString& _store_id);
        virtual ~StickerPackInfo();

        void show();

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

    private:

        std::unique_ptr<GeneralDialog> parentDialog_;
    };
}