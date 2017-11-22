#pragma once

namespace Utils
{
    class SignalsDisconnector;
}

namespace Ui
{
    class qt_gui_settings;
    class SemitransparentWindowAnimated;

    class GeneralDialog : public QDialog
    {
        Q_OBJECT
            Q_SIGNALS:
        void leftButtonClicked();
        void rightButtonClicked();
        void shown(QWidget *);
        void hidden(QWidget *);
        void moved(QWidget *);
        void resized(QWidget *);

    private Q_SLOTS:
        void leftButtonClick();
        void rightButtonClick();

    public Q_SLOTS:
        void setButtonActive(bool _active);
        virtual void reject() override;

    public:
        GeneralDialog(QWidget* _mainWidget, QWidget* _parent, bool _ignoreKeyPressEvents = false);
        ~GeneralDialog();
        bool showInPosition(int _x, int _y);
        QWidget* getMainHost();

        void addAcceptButton(QString _buttonText, const bool _isEnabled);
        void addCancelButton(QString _buttonText);
        void addButtonsPair(QString _buttonTextLeft, QString _buttonTextRight, bool _isActive, bool _rejectable = true, bool _acceptable = true);

        QPushButton* takeAcceptButton();

        void addLabel(const QString& _text);
        void addHead();
        void addText(QString _messageText, int _upperMarginPx);
        void addError(QString _messageText);
        void setKeepCenter(bool _isKeepCenter);
        inline void setShadow(bool b) { shadow_ = b; }

        inline void setLeftButtonDisableOnClicked(bool v) { leftButtonDisableOnClicked_ = v; }
        inline void setRightButtonDisableOnClicked(bool v) { rightButtonDisableOnClicked_ = v; }

        void updateSize();

    protected:
        virtual void showEvent(QShowEvent *) override;
        virtual void hideEvent(QHideEvent *) override;
        virtual void moveEvent(QMoveEvent *) override;
        virtual void resizeEvent(QResizeEvent *) override;
        virtual void mousePressEvent(QMouseEvent* _e) override;
        virtual void keyPressEvent(QKeyEvent* _e) override;

    private:
        QLayout *initBottomLayout();

        void moveToPosition(int _x, int _y);

    private:
        QWidget*                        mainHost_;
        double                          koeffHeight_;
        double                          koeffWidth_;
        QWidget*                        mainWidget_;
        int                             addWidth_;
        QPushButton*                    nextButton_;
        SemitransparentWindowAnimated*  semiWindow_;
        QWidget*                        bottomWidget_;
        QWidget*                        textHost_;
        QWidget*                        errorHost_;
        QWidget*                        headerBtnHost_;
        QWidget*                        headerLabelHost_;
        bool                            keepCenter_;
        int                             x_;
        int                             y_;
        bool                            ignoreKeyPressEvents_;
        bool                            shadow_;

        bool                            leftButtonDisableOnClicked_;
        bool                            rightButtonDisableOnClicked_;
    };
}
