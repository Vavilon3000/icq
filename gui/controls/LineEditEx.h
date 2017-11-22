#pragma once


namespace Ui
{
    class LineEditEx : public QLineEdit
    {
        Q_OBJECT
Q_SIGNALS:
        void focusIn();
        void focusOut();
        void clicked();
        void emptyTextBackspace();
        void escapePressed();
        void upArrow();
        void downArrow();
        void enter();

    public:
        explicit LineEditEx(QWidget* _parent);

    protected:
        virtual void focusInEvent(QFocusEvent*) override;
        virtual void focusOutEvent(QFocusEvent*) override;
        virtual void mousePressEvent(QMouseEvent*) override;
        virtual void keyPressEvent(QKeyEvent*) override;
        virtual void contextMenuEvent(QContextMenuEvent *) override;

    };
}