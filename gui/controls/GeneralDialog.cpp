#include "stdafx.h"

#include "CommonStyle.h"
#include "GeneralDialog.h"
#include "TextEmojiWidget.h"
#include "TextEditEx.h"
#include "SemitransparentWindowAnimated.h"
#include "CustomButton.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../main_window/MainWindow.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"

namespace
{
    constexpr int DURATION = 100;
}

namespace Ui
{
    GeneralDialog::GeneralDialog(QWidget* _mainWidget, QWidget* _parent, bool _ignoreKeyPressEvents/* = false*/)
        : QDialog(nullptr)
        , mainWidget_(_mainWidget)
        , nextButton_(nullptr)
        , semiWindow_(new SemitransparentWindowAnimated(_parent, DURATION))
        , headerBtnHost_(nullptr)
        , headerLabelHost_(nullptr)
        , keepCenter_(true)
        , x_(-1)
        , y_(-1)
        , ignoreKeyPressEvents_(_ignoreKeyPressEvents)
        , shadow_(true)
        , leftButtonDisableOnClicked_(false)
        , rightButtonDisableOnClicked_(false)
    {
        setParent(semiWindow_);
        setFocus();

        semiWindow_->hide();

        auto mainLayout = Utils::emptyVLayout(this);
        mainHost_ = new QWidget(this);
        mainHost_->setObjectName(qsl("mainHost"));

        auto globalLayout = Utils::emptyVLayout(mainHost_);

        auto headerHost = new QWidget(mainHost_);
        auto headerLayout = Utils::emptyHLayout(mainHost_);

        headerLabelHost_ = new QWidget(mainHost_);
        headerLabelHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        headerLabelHost_->setVisible(false);
        headerLayout->addWidget(headerLabelHost_);

        headerBtnHost_ = new QWidget(mainHost_);
        headerBtnHost_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        headerBtnHost_->setVisible(false);
        headerLayout->addWidget(headerBtnHost_);

        headerHost->setLayout(headerLayout);
        globalLayout->addWidget(headerHost);

        errorHost_ = new QWidget(mainHost_);
        errorHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::ApplyStyle(errorHost_, qsl("height: 1dip;"));
        errorHost_->setVisible(false);
        globalLayout->addWidget(errorHost_);

        textHost_ = new QWidget(mainHost_);
        textHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::ApplyStyle(textHost_, qsl("height: 1dip;"));
        textHost_->setVisible(false);
        globalLayout->addWidget(textHost_);

        if (mainWidget_)
        {
            globalLayout->addWidget(mainWidget_);
        }

        bottomWidget_ = new QWidget(mainHost_);
        bottomWidget_->setVisible(false);
        globalLayout->addWidget(bottomWidget_);

        setStyleSheet(qsl("background-color: %1;")
        .arg(CommonStyle::getFrameColor().name()));

        setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        QWidget::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, &GeneralDialog::reject);
        QWidget::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupWindow, this, &GeneralDialog::reject);

        mainLayout->setSizeConstraint(QLayout::SetFixedSize);
        mainLayout->addWidget(mainHost_);
    }

    void GeneralDialog::reject()
    {
        semiWindow_->Hide();
        QDialog::reject();
    }

    void GeneralDialog::addLabel(const QString& _text)
    {
        headerLabelHost_->setVisible(true);
        auto hostLayout = Utils::emptyHLayout(headerLabelHost_);
        hostLayout->setContentsMargins(Utils::scale_value(16), 0, 0, 0);
        hostLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        headerLabelHost_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);

        auto label = new TextEmojiWidget(headerLabelHost_, Fonts::appFontScaled(22), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY), Utils::scale_value(32));
        label->setText(_text);
        label->setEllipsis(true);
        hostLayout->addWidget(label);
    }

    void GeneralDialog::addText(QString _messageText, int _upperMarginPx)
    {
        textHost_->setVisible(true);
        auto textLayout = Utils::emptyVLayout(textHost_);

        auto label = new Ui::TextEditEx(textHost_, Fonts::appFontScaled(15), CommonStyle::getColor(CommonStyle::Color::TEXT_SECONDARY), true, true);
        label->setFixedHeight(Utils::scale_value(16));
        label->setContentsMargins(0, 0, 0, 0);
        label->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        label->setPlaceholderText(QString());
        label->setAutoFillBackground(false);
        label->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        label->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        {
            const QString ls = qsl("QWidget { border: none; padding-left: 16dip; padding-right: 16dip; padding-top: 0dip; padding-bottom: 0dip; }");
            Utils::ApplyStyle(label, ls);
        }

        auto upperSpacer = new QSpacerItem(0, _upperMarginPx, QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer);

        label->setText(_messageText);
        textLayout->addWidget(label);
    }

    void GeneralDialog::addAcceptButton(QString _buttonText, const bool _isEnabled)
    {
        bottomWidget_->setVisible(true);

        auto bottomLayout = initBottomLayout();

        bottomWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Policy::Preferred);
        {
            nextButton_ = new QPushButton(bottomWidget_);

            Utils::ApplyStyle(
                nextButton_,
                _isEnabled ?
                    CommonStyle::getGreenButtonStyle() :
                    CommonStyle::getDisabledButtonStyle());

            setButtonActive(_isEnabled);
            nextButton_->setFlat(true);
            nextButton_->setCursor(QCursor(Qt::PointingHandCursor));
            nextButton_->setText(_buttonText);
            nextButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            nextButton_->adjustSize();

            QObject::connect(nextButton_, &QPushButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);

            auto buttonLayout = Utils::emptyVLayout(bottomWidget_);
            buttonLayout->setAlignment(Qt::AlignHCenter);
            buttonLayout->addWidget(nextButton_);
            bottomLayout->addItem(buttonLayout);
        }

        Testing::setAccessibleName(nextButton_, qsl("nextButton_"));
    }

    void GeneralDialog::addCancelButton(QString _buttonText)
    {
        bottomWidget_->setVisible(true);

        auto bottomLayout = initBottomLayout();

        bottomWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Policy::Preferred);
        {
            nextButton_ = new QPushButton(bottomWidget_);

            Utils::ApplyStyle(nextButton_, CommonStyle::getGrayButtonStyle());

            nextButton_->setFlat(true);
            nextButton_->setCursor(QCursor(Qt::PointingHandCursor));
            nextButton_->setText(_buttonText);
            nextButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            nextButton_->adjustSize();

            QObject::connect(nextButton_, &QPushButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);

            auto buttonLayout = Utils::emptyVLayout(bottomWidget_);
            buttonLayout->setAlignment(Qt::AlignHCenter);
            buttonLayout->addWidget(nextButton_);
            bottomLayout->addItem(buttonLayout);
        }

        Testing::setAccessibleName(nextButton_, qsl("nextButton_"));
    }

    void GeneralDialog::addButtonsPair(QString _buttonTextLeft, QString _buttonTextRight, bool _isActive, bool _rejectable, bool _acceptable)
    {
        bottomWidget_->setVisible(true);
        auto bottomLayout = Utils::emptyHLayout(bottomWidget_);

        bottomLayout->setContentsMargins(Utils::scale_value(16), Utils::scale_value(16), Utils::scale_value(16), Utils::scale_value(16));
        bottomLayout->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
        bottomWidget_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        {
            auto cancelButton = new QPushButton(bottomWidget_);
            Utils::ApplyStyle(cancelButton, CommonStyle::getGrayButtonStyle());
            cancelButton->setAccessibleName(qsl("left_button"));
            cancelButton->setFlat(true);
            cancelButton->setCursor(QCursor(Qt::PointingHandCursor));
            cancelButton->setText(_buttonTextLeft);
            cancelButton->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            QObject::connect(cancelButton, &QPushButton::clicked, this, &GeneralDialog::leftButtonClick, Qt::QueuedConnection);
            if (_rejectable)
                QObject::connect(cancelButton, &QPushButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);
            bottomLayout->addWidget(cancelButton);

            auto betweenSpacer = new QSpacerItem(Utils::scale_value(16), 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            bottomLayout->addItem(betweenSpacer);

            nextButton_ = new QPushButton(bottomWidget_);
            Utils::ApplyStyle(nextButton_, CommonStyle::getGreenButtonStyle());
            setButtonActive(_isActive);
            nextButton_->setFlat(true);
            nextButton_->setCursor(QCursor(Qt::PointingHandCursor));
            nextButton_->setText(_buttonTextRight);
            nextButton_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            QObject::connect(nextButton_, &QPushButton::clicked, this, &GeneralDialog::rightButtonClick, Qt::QueuedConnection);
            if (_acceptable)
                QObject::connect(nextButton_, &QPushButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);
            bottomLayout->addWidget(nextButton_);
        }

        Testing::setAccessibleName(nextButton_, qsl("nextButton_"));
    }

    QPushButton* GeneralDialog::takeAcceptButton()
    {
        if (nextButton_)
            QObject::disconnect(nextButton_, &QPushButton::clicked, this, &GeneralDialog::accept);

        return nextButton_;
    }

    void GeneralDialog::addHead()
    {
        headerLabelHost_->setVisible(true);
        headerBtnHost_->setVisible(true);
        auto hostLayout = Utils::emptyHLayout(headerBtnHost_);
        hostLayout->setAlignment(Qt::AlignRight | Qt::AlignTop);
        headerBtnHost_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);

        auto btn = new CustomButton(headerBtnHost_, qsl(":/controls/close_big_e_100"));
        btn->setFixedWidth(Utils::scale_value(48));
        btn->setFixedHeight(Utils::scale_value(48));
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);
        hostLayout->addWidget(btn);
    }

    GeneralDialog::~GeneralDialog()
    {
        semiWindow_->setParent(0);
        semiWindow_->deleteLater();
    }

    void GeneralDialog::setKeepCenter(bool _isKeepCenter)
    {
        keepCenter_ = _isKeepCenter;
    }

    bool GeneralDialog::showInPosition(int _x, int _y)
    {
        if (shadow_)
        {
            shadow_ = false;
            Utils::addShadowToWindow(this, true);
        }
        x_ = _x;
        y_ = _y;
        semiWindow_->Show();
        show();
        auto result = (exec() == QDialog::Accepted);
        close();
        if (platform::is_apple())
            semiWindow_->parentWidget()->activateWindow();
        return result;
    }

    QWidget* GeneralDialog::getMainHost()
    {
        return mainHost_;
    }

    void GeneralDialog::setButtonActive(bool _active)
    {
        if (!nextButton_)
        {
            return;
        }

        Utils::ApplyStyle(nextButton_, _active ?
            CommonStyle::getGreenButtonStyle() : CommonStyle::getDisabledButtonStyle());

        nextButton_->setEnabled(_active);
    }

    QLayout* GeneralDialog::initBottomLayout()
    {
        assert(bottomWidget_);

        if (bottomWidget_->layout())
        {
            assert(qobject_cast<QVBoxLayout*>(bottomWidget_->layout()));
            return bottomWidget_->layout();
        }

        auto bottomLayout = Utils::emptyVLayout(bottomWidget_);

        bottomLayout->setContentsMargins(0, Utils::scale_value(16), 0, Utils::scale_value(16));
        bottomLayout->setAlignment(Qt::AlignBottom);

        return bottomLayout;
    }

    void GeneralDialog::moveToPosition(int _x, int _y)
    {
        if (_x == -1 && _y == -1)
        {
            QRect r;
            if (semiWindow_->isMainWindow())
            {
                auto mainWindow = Utils::InterConnector::instance().getMainWindow();
                int titleHeight = mainWindow->getTitleHeight();
                r = QRect(0, 0, mainWindow->width(), mainWindow->height() - titleHeight);
            }
            else
            {
                r = parentWidget()->geometry();
            }
            auto corner = r.center();
            auto size = sizeHint();
            move(corner.x() - size.width() / 2, corner.y() - size.height() / 2);
        }
        else
        {
            move(_x, _y);
        }
    }

    void GeneralDialog::mousePressEvent(QMouseEvent* _e)
    {
        QDialog::mousePressEvent(_e);
        if (!geometry().contains(mapToParent(_e->pos())))
            close();
        else
            _e->accept();
    }

    void GeneralDialog::keyPressEvent(QKeyEvent* _e)
    {
        if (_e->key() == Qt::Key_Escape)
        {
            close();
            return;
        }

        if (_e->key() == Qt::Key_Return || _e->key() == Qt::Key_Enter)
        {
            if (!ignoreKeyPressEvents_ && nextButton_->isEnabled())
            {
                accept();
            }
            else
            {
                _e->ignore();
            }
            return;
        }

        QDialog::keyPressEvent(_e);
    }

    void GeneralDialog::showEvent(QShowEvent *event)
    {
        QDialog::showEvent(event);
        emit shown(this);
    }

    void GeneralDialog::hideEvent(QHideEvent *event)
    {
        emit hidden(this);
        QDialog::hideEvent(event);
    }

    void GeneralDialog::moveEvent(QMoveEvent *_event)
    {
        QDialog::moveEvent(_event);
        emit moved(this);
    }

    void GeneralDialog::resizeEvent(QResizeEvent *_event)
    {
        QDialog::resizeEvent(_event);
        if (semiWindow_)
        {
            semiWindow_->updateSize();
        }
        if (keepCenter_)
        {
            moveToPosition(x_, y_);
        }
        emit resized(this);
    }

    void GeneralDialog::addError(QString _messageText)
    {
        errorHost_->setVisible(true);
        errorHost_->setContentsMargins(0, 0, 0, 0);
        errorHost_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);

        auto textLayout = Utils::emptyVLayout(errorHost_);

        auto upperSpacer = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer);

        const QString backgroundStyle = qsl("background-color: #fbdbd9; ");
        const QString labelStyle = ql1s("QWidget { ") % backgroundStyle % ql1s("border: none; padding-left: 16dip; padding-right: 16dip; padding-top: 0dip; padding-bottom: 0dip; }");

        auto upperSpacerRedUp = new QLabel();
        upperSpacerRedUp->setFixedHeight(Utils::scale_value(16));
        Utils::ApplyStyle(upperSpacerRedUp, backgroundStyle);
        textLayout->addWidget(upperSpacerRedUp);

        auto errorLabel = new Ui::TextEditEx(errorHost_, Fonts::appFontScaled(16), CommonStyle::getColor(CommonStyle::Color::TEXT_RED), true, true);
        errorLabel->setContentsMargins(0, 0, 0, 0);
        errorLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        errorLabel->setPlaceholderText(QString());
        errorLabel->setAutoFillBackground(false);
        errorLabel->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        errorLabel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(errorLabel, labelStyle);

        errorLabel->setText(QT_TRANSLATE_NOOP("popup_window", "Unfortunately, an error occurred:"));
        textLayout->addWidget(errorLabel);

        auto errorText = new Ui::TextEditEx(errorHost_, Fonts::appFontScaled(16), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY), true, true);
        errorText->setContentsMargins(0, 0, 0, 0);
        errorText->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        errorText->setPlaceholderText(QString());
        errorText->setAutoFillBackground(false);
        errorText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        errorText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(errorText, labelStyle);

        errorText->setText(_messageText);
        textLayout->addWidget(errorText);

        auto upperSpacerRedBottom = new QLabel();
        Utils::ApplyStyle(upperSpacerRedBottom, backgroundStyle);
        upperSpacerRedBottom->setFixedHeight(Utils::scale_value(16));
        textLayout->addWidget(upperSpacerRedBottom);

        auto upperSpacer2 = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer2);
    }

    void GeneralDialog::leftButtonClick()
    {
        if (auto leftButton = bottomWidget_->findChild<QPushButton *>(qsl("left_button")))
            leftButton->setEnabled(!leftButtonDisableOnClicked_);
        emit leftButtonClicked();
    }

    void GeneralDialog::rightButtonClick()
    {
        nextButton_->setEnabled(!rightButtonDisableOnClicked_);
        emit rightButtonClicked();
    }

    void GeneralDialog::updateSize()
    {
        if (semiWindow_)
        {
            semiWindow_->updateSize();
        }
        if (keepCenter_)
        {
            moveToPosition(x_, y_);
        }
        setFixedSize(sizeHint());
    }
}
