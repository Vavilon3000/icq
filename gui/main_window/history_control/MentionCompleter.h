#pragma once

namespace Logic
{
    class MentionModel;
    class MentionItemDelegate;
}

namespace Ui
{
    class FocusableListView;

    class MentionCompleter : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void contactSelected(const QString& _aimId, const QString& _friendly);
        void results(int _count);
        void hidden();

    public Q_SLOTS:
        void beforeShow();

    private Q_SLOTS:
        void itemClicked(const QModelIndex& _current);
        void onResults();

    private:
        Logic::MentionModel* model_;
        Logic::MentionItemDelegate* delegate_;
        FocusableListView* view_;

    public:
        MentionCompleter(QWidget* _parent);

        void setSearchPattern(const QString& _pattern);
        QString getSearchPattern() const;

        void setDialogAimId(const QString& _aimid);
        bool insertCurrent();
        void keyUpOrDownPressed(const bool _isUpPressed);
        void recalcHeight();

        bool completerVisible() const;
        int itemCount() const;
        bool hasSelectedItem() const;

    protected:
        virtual void hideEvent(QHideEvent* _event) override;
        virtual void keyPressEvent(QKeyEvent* _event) override;

    private:
        int calcHeight() const;
    };
}