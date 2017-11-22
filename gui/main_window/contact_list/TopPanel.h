#pragma once

#include "../../cache/avatars/AvatarStorage.h"

namespace Ui
{
    class CustomButton;
    class SearchWidget;
    class UnreadMailWidget;

    class BurgerWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void clicked();
        void back();

    public:
        BurgerWidget(QWidget* parent);
        void setBack(bool _back);

    protected:
        virtual void paintEvent(QPaintEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);

    private:
        bool Back_;
    };

    class TopPanelWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void back();
        void burgerClicked();

    public:
        enum Mode
        {
            NORMAL = 0,
            COMPACT = 1,
            SPREADED = 2,
        };

        TopPanelWidget(QWidget* parent, SearchWidget* searchWidget);

        void setMode(Mode _mode);
        void setBack(bool _back);
        void searchActivityChanged(bool _active);

	    void addWidget(QWidget* w);

    protected:
        virtual void paintEvent(QPaintEvent* _e) override;

    private Q_SLOTS:
        void searchActiveChanged(bool);
        void titleButtonsUpdated();

    private:
        BurgerWidget* Burger_;
        QWidget* LeftSpacer_;
        QWidget* RightSpacer_;
        QWidget* AdditionalSpacerLeft_;
        QWidget* AdditionalSpacerRight_;
        QHBoxLayout* mainLayout;
        SearchWidget* Search_;
        Mode Mode_;
	    QHBoxLayout* Additional_;
    };
}
