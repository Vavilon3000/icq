#pragma once

#include "../../controls/TextEditEx.h"
#include "../../types/message.h"

class QPushButton;

namespace Ui
{
    class PictureWidget;
    class LabelEx;
    class MentionCompleter;

    enum DisableReason
    {
        ReadOnlyChat = 0,
        NotAMember = 1,
    };

    class HistoryTextEdit : public TextEditEx
    {
    public:
        HistoryTextEdit(QWidget* _parent);
        virtual bool catchEnter(const int _modifiers) override;
        virtual bool catchNewLine(const int _modifiers) override;
    protected:
        virtual void keyPressEvent(QKeyEvent*) override;
    };

    class InputWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void mentionSignPressed(const QString& _dialogAimId);

        void smilesMenuSignal();
        void sendMessage(const QString&);
        void editFocusOut();
        void ctrlFPressedInInputWidget();
        void needUpdateSizes();
        void sizeChanged();
        void inputTyped();

    public Q_SLOTS:
        void quote(const QVector<Data::Quote>&);
        void contactSelected(const QString&);
        void insert_emoji(int32_t _main, int32_t _ext);
        void send_sticker(int32_t _set_id, int32_t _sticker_id);
        void smilesMenuHidden();

        void insertMention(const QString& _aimId, const QString& _friendly);

    private Q_SLOTS:
        void editContentChanged();
        void cursorPositionChanged();
        void enterPressed();
        void send();
        void attach_file();
        void stats_attach_file();
        void clearQuotes(const QString&);
        void clearFiles(const QString&);
        void clearMentions(const QString&);
        void clear();
        void updateSizes();

        void resize_to(int _height);

        void typed();

        void statsMessageEnter();
        void statsMessageSend();

        void onFilesCancel();
        void disableActionClicked(const QString&);
        void chatRoleChanged(QString);

    public:

        InputWidget(QWidget* parent, int height = -1, const QString& styleSheet = QString(), bool messageOnly = false);

        void hide();
        void hideNoClear();

        void disable(DisableReason reason);
        void enable();

        void predefined(const QString& _contact, const QString& _text);

        Q_PROPERTY(int current_height READ get_current_height WRITE set_current_height)

        void set_current_height(int _val);
        int get_current_height() const;

        void setFocusOnInput();

        void setFontColor(const QColor& _color);

    private:
        void initQuotes(const QString&);
        void initFiles(const QString&);
        QPixmap getFilePreview(const QString& contact);
        QString getFileSendText(const QString& contact) const;

        bool isAllAttachmentsEmpty(const QString& contact) const;

        bool shouldOfferMentions() const;
        MentionCompleter* getMentionCompleter();

    private:

        QPushButton* smiles_button_;
        QPushButton* send_button_;
        QPushButton* file_button_;
        QPropertyAnimation* anim_height_;

        HistoryTextEdit* text_edit_;
        QString contact_;
        QString predefined_;

        QWidget* quote_text_widget_;
        TextEditEx* quote_text_;
        QWidget* quote_line_;
        QWidget* quote_block_;
        QWidget* files_block_;
        QWidget* input_disabled_block_;
        QPushButton* cancel_quote_;
        QPushButton* cancel_files_;
        QMap<QString, QVector<Data::Quote>> quotes_;
        QMap<QString, QStringList> files_to_send_;
        PictureWidget* file_preview_;
        LabelEx* files_label_;
        LabelEx* disable_label_;
        QMap<QString, QPixmap> image_buffer_;

        QMap<QString, Data::MentionMap> mentions_;

        int active_height_;
        int need_height_;
        int active_document_height_;
        int default_height_;
        bool is_initializing_;
        bool message_only_;
        int mentionSignIndex_;
        QMap<QString, DisableReason>  disabled_;

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;
        virtual void resizeEvent(QResizeEvent * _e) override;
        virtual void keyPressEvent(QKeyEvent * _e) override;
        virtual void keyReleaseEvent(QKeyEvent * _e) override;
    };
}