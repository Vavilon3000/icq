#include "stdafx.h"
#include "ContactDialog.h"

#include "MainWindow.h"
#include "contact_list/ContactListModel.h"
#include "history_control/HistoryControl.h"
#include "input_widget/InputWidget.h"
#include "sidebar/Sidebar.h"
#include "smiles_menu/SmilesMenu.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../controls/CommonStyle.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/InterConnector.h"

namespace Ui
{
    DragOverlayWindow::DragOverlayWindow(ContactDialog* _parent)
        : QWidget(_parent)
        , Parent_(_parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAcceptDrops(true);
    }

    void DragOverlayWindow::paintEvent(QPaintEvent *)
    {
        QPainter painter(this);

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setBrush(QBrush(Qt::transparent));

        QColor overlayColor(CommonStyle::getFrameColor());
        overlayColor.setAlphaF(0.9);

        painter.fillRect(
            rect().x(),
            rect().y(),
            rect().width(),
            CommonStyle::getTopPanelHeight(),
            QBrush(Qt::transparent)
        );
        painter.fillRect(
            rect().x(),
            rect().y() + CommonStyle::getTopPanelHeight(),
            rect().width(),
            rect().height() - CommonStyle::getTopPanelHeight(),
            QBrush(overlayColor)
        );

        QPen pen (CommonStyle::getColor(CommonStyle::Color::GREEN_FILL), Utils::scale_value(2), Qt::DashLine, Qt::RoundCap);
        painter.setPen(pen);
        painter.drawRoundedRect(
            Utils::scale_value(24),
            CommonStyle::getTopPanelHeight() + Utils::scale_value(24),
            rect().width() - Utils::scale_value(24) * 2,
            rect().height() - CommonStyle::getTopPanelHeight() - Utils::scale_value(24) * 2,
            Utils::scale_value(8),
            Utils::scale_value(8)
        );

        QPixmap p(Utils::parse_image_name(qsl(":/resources/file_sharing/upload_main_100.png")));
        Utils::check_pixel_ratio(p);
        double ratio = Utils::scale_bitmap(1);
        int x = (rect().width() / 2) - (p.width() / 2. / ratio);
        int y = (rect().height() / 2) - (p.height() / 2. / ratio);
        painter.drawPixmap(x, y, p);
        painter.setFont(Fonts::appFontScaled(15));
        Utils::drawText(
            painter,
            QPointF(rect().width() / 2,
                y + p.height() + Utils::scale_value(24)),
            Qt::AlignHCenter | Qt::AlignVCenter,
            QT_TRANSLATE_NOOP("chat_page", "Drop files to place")
        );
    }

    void DragOverlayWindow::dragEnterEvent(QDragEnterEvent *_e)
    {
        _e->acceptProposedAction();
    }

    void DragOverlayWindow::dragLeaveEvent(QDragLeaveEvent *_e)
    {
        hide();
        _e->accept();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void DragOverlayWindow::dragMoveEvent(QDragMoveEvent *_e)
    {
        _e->acceptProposedAction();
    }

    void DragOverlayWindow::dropEvent(QDropEvent *_e)
    {
        const QMimeData* mimeData = _e->mimeData();

        if (mimeData->hasUrls())
        {
            const QList<QUrl> urlList = mimeData->urls();

            const QString contact = Logic::getContactListModel()->selectedContact();
            for (const QUrl& url : urlList)
            {
                if (url.isLocalFile())
                {
                    QFileInfo info(url.toLocalFile());
                    bool canDrop = !(info.isBundle() || info.isDir());
                    if (info.size() == 0)
                        canDrop = false;

                    if (canDrop)
                    {
                        Ui::GetDispatcher()->uploadSharedFile(contact, url.toLocalFile());
                        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dnd_dialog);
                        Parent_->onSendMessage(contact);
                    }
                }
                else if (url.isValid())
                {
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("contact", contact);
                    const auto text = url.toString().toUtf8();
                    collection.set_value_as_string("message", text.data(), text.size());
                    Ui::GetDispatcher()->post_message_to_core(qsl("send_message"), collection.get());
                    Parent_->onSendMessage(contact);
                }
            }
        }

        _e->acceptProposedAction();
        hide();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

	ContactDialog::ContactDialog(QWidget* _parent)
		:	QWidget(_parent)
		, historyControlWidget_(new HistoryControl(this))
		, inputWidget_(new InputWidget(this))
		, smilesMenu_(new Smiles::SmilesMenu(this))
        , dragOverlayWindow_(new DragOverlayWindow(this))
        , sidebar_(new Sidebar(_parent))
        , overlayUpdateTimer_(new QTimer(this))
        , topWidget_(new QStackedWidget(this))
        , rootLayout_(Utils::emptyVLayout())
	{
        setAcceptDrops(true);
        topWidget_->setFixedHeight(CommonStyle::getTopPanelHeight());
        rootLayout_->addWidget(topWidget_);
        rootLayout_->addWidget(historyControlWidget_);
        rootLayout_->addWidget(smilesMenu_);
        rootLayout_->addWidget(inputWidget_);
		rootLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum));
		setLayout(rootLayout_);

        Testing::setAccessibleName(topWidget_, qsl("AS topWidget_"));
        Testing::setAccessibleName(historyControlWidget_, qsl("AS historyControlWidget_"));
        Testing::setAccessibleName(smilesMenu_, qsl("AS smilesMenu_"));
        Testing::setAccessibleName(inputWidget_, qsl("AS inputWidget_"));

        topWidget_->hide();
        sidebar_->hide();

		connect(inputWidget_, &InputWidget::smilesMenuSignal, this, &ContactDialog::onSmilesMenu, Qt::QueuedConnection);
		connect(inputWidget_, &InputWidget::editFocusOut, this, &ContactDialog::onInputEditFocusOut, Qt::QueuedConnection);
		connect(inputWidget_, &InputWidget::sendMessage, this, &ContactDialog::onSendMessage, Qt::QueuedConnection);
        connect(inputWidget_, &InputWidget::inputTyped, this, &ContactDialog::inputTyped);
		connect(inputWidget_, &InputWidget::ctrlFPressedInInputWidget, this, &ContactDialog::onCtrlFPressedInInputWidget, Qt::QueuedConnection);

		connect(this, &ContactDialog::contactSelected, inputWidget_, &InputWidget::contactSelected, Qt::QueuedConnection);
        connect(this, &ContactDialog::contactSelected, historyControlWidget_, &HistoryControl::contactSelected, Qt::QueuedConnection);
		connect(this, &ContactDialog::contactSelectedToLastMessage, historyControlWidget_, &HistoryControl::contactSelectedToLastMessage, Qt::QueuedConnection);

        connect(historyControlWidget_, &HistoryControl::quote, inputWidget_, &InputWidget::quote, Qt::QueuedConnection);
        connect(historyControlWidget_, &HistoryControl::clicked, this, &ContactDialog::historyControlClicked, Qt::QueuedConnection);

        connect(smilesMenu_, &Smiles::SmilesMenu::menuHidden, inputWidget_, &InputWidget::smilesMenuHidden, Qt::QueuedConnection);

		initSmilesMenu();
		initInputWidget();

        overlayUpdateTimer_->setInterval(500);
        overlayUpdateTimer_->setSingleShot(false);
        connect(overlayUpdateTimer_, &QTimer::timeout, this, &ContactDialog::updateDragOverlay, Qt::QueuedConnection);

        dragOverlayWindow_->move(0,0);
        dragOverlayWindow_->hide();
	}

	ContactDialog::~ContactDialog()
	{
        delete topWidget_;
        topWidget_ = nullptr;
	}

    void ContactDialog::showSidebar(const QString& _aimId, int _page)
    {
        sidebar_->preparePage(_aimId, _page == all_members ? menu_page : (SidebarPages)_page);
        setSidebarVisible(true);
        if (_page == all_members)
            sidebar_->showAllMembers();
    }

    void ContactDialog::setSidebarVisible(bool _show)
    {
        if (sidebar_->isVisible() == _show)
            return;

        if (_show)
            sidebar_->showAnimated();
        else
            sidebar_->hideAnimated();
    }

    bool ContactDialog::isSidebarVisible() const
    {
        return sidebar_->isVisible();
    }

	void ContactDialog::onContactSelected(const QString& _aimId, qint64 _messageId, qint64 _quoteId)
	{
		emit contactSelected(_aimId, _messageId, _quoteId);
	}

	void ContactDialog::onContactSelectedToLastMessage(QString _aimId, qint64 _messageId)
	{
		emit contactSelectedToLastMessage(_aimId, _messageId);
	}

	void ContactDialog::initSmilesMenu()
	{
		smilesMenu_->setFixedHeight(0);

		connect(smilesMenu_, &Smiles::SmilesMenu::emojiSelected,   inputWidget_, &InputWidget::insert_emoji);
		connect(smilesMenu_, &Smiles::SmilesMenu::stickerSelected, inputWidget_, &InputWidget::send_sticker);
	}

	void ContactDialog::initInputWidget()
	{
		connect(inputWidget_, &InputWidget::sendMessage, this, [this]()
		{
			smilesMenu_->Hide();
		});
	}

	void ContactDialog::onSmilesMenu()
	{
		smilesMenu_->ShowHide();
	}

	void ContactDialog::onInputEditFocusOut()
	{
		smilesMenu_->Hide();
	}

    void ContactDialog::hideSmilesMenu()
    {
        smilesMenu_->Hide();
    }

    void ContactDialog::updateDragOverlay()
    {
        if (!rect().contains(mapFromGlobal(QCursor::pos())))
        {
            hideDragOverlay();
        }
    }

    void ContactDialog::historyControlClicked()
    {
        emit clicked();
    }

    void ContactDialog::onSendMessage(const QString& _contact)
    {
        historyControlWidget_->scrollHistoryToBottom(_contact);

        emit sendMessage(_contact);
    }

    void ContactDialog::cancelSelection()
    {
        assert(historyControlWidget_);
        historyControlWidget_->cancelSelection();
    }

    void ContactDialog::hideInput()
    {
        setSidebarVisible(false);
        overlayUpdateTimer_->stop();
        inputWidget_->hide();
        topWidget_->hide();
    }

    void ContactDialog::showDragOverlay()
    {
        dragOverlayWindow_->show();
        overlayUpdateTimer_->start();
        Utils::InterConnector::instance().setDragOverlay(true);
    }

    void ContactDialog::hideDragOverlay()
    {
        dragOverlayWindow_->hide();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void ContactDialog::insertTopWidget(const QString& _aimId, QWidget* _widget)
    {
        if (!topWidgetsCache_.contains(_aimId))
        {
            topWidgetsCache_.insert(_aimId, _widget);
            topWidget_->addWidget(_widget);
        }

        topWidget_->show();
        topWidget_->setCurrentWidget(topWidgetsCache_[_aimId]);
    }

    void ContactDialog::removeTopWidget(const QString& _aimId)
    {
        if (!topWidget_)
            return;

        if (topWidgetsCache_.contains(_aimId))
        {
            topWidget_->removeWidget(topWidgetsCache_[_aimId]);
            topWidgetsCache_.remove(_aimId);
        }

        if (!topWidget_->currentWidget())
            topWidget_->hide();
    }

    void ContactDialog::dragEnterEvent(QDragEnterEvent *_e)
    {
        if (Logic::getContactListModel()->selectedContact().isEmpty() || !(_e->mimeData() && _e->mimeData()->hasUrls()) || _e->mimeData()->property("icq").toBool())
        {
            _e->setDropAction(Qt::IgnoreAction);
            return;
        }

        auto role = Logic::getContactListModel()->getYourRole(Logic::getContactListModel()->selectedContact());
        if (role == ql1s("notamember") || role == ql1s("readonly"))
        {
            _e->setDropAction(Qt::IgnoreAction);
            return;
        }

        Utils::InterConnector::instance().getMainWindow()->closeGallery();
        Utils::InterConnector::instance().getMainWindow()->activate();
        if (!dragOverlayWindow_->isVisible())
            showDragOverlay();
        _e->acceptProposedAction();
    }

    void ContactDialog::dragLeaveEvent(QDragLeaveEvent *_e)
    {
        _e->accept();
    }

    void ContactDialog::dragMoveEvent(QDragMoveEvent *_e)
    {
        _e->acceptProposedAction();
    }

    void ContactDialog::resizeEvent(QResizeEvent* _e)
    {
        sidebar_->updateSize();
        dragOverlayWindow_->resize(width(), height());
        historyControlWidget_->setFixedWidth(_e->size().width());
        inputWidget_->setFixedWidth(_e->size().width());
        smilesMenu_->setFixedWidth(_e->size().width());
        QWidget::resizeEvent(_e);
    }

    HistoryControlPage* ContactDialog::getHistoryPage(const QString& _aimId) const
    {
        return historyControlWidget_->getHistoryPage(_aimId);
	}

    const Smiles::SmilesMenu* ContactDialog::getSmilesMenu() const
    {
        assert(smilesMenu_);
        return smilesMenu_;
    }

    void ContactDialog::setFocusOnInputWidget()
    {
        if (!Logic::getContactListModel()->selectedContact().isEmpty())
            inputWidget_->setFocusOnInput();
    }

    Ui::InputWidget* ContactDialog::getInputWidget() const
    {
        return inputWidget_;
    }

    void ContactDialog::notifyApplicationWindowActive(const bool isActive)
    {
        if (historyControlWidget_)
        {
            historyControlWidget_->notifyApplicationWindowActive(isActive);
        }
    }

    void ContactDialog::onCtrlFPressedInInputWidget()
    {
        emit Utils::InterConnector::instance().setSearchFocus();
    }

    void ContactDialog::inputTyped()
    {
        historyControlWidget_->inputTyped();
    }
}

