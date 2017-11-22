#pragma once
#include "Sidebar.h"
#include "../../types/chat.h"
#include "../../../corelib/enumerations.h"

namespace Ui
{
    class ContactAvatarWidget;
    class CustomButton;
    class TextEditEx;
    class LabelEx;
    class LiveChatMembersControl;
    class TextEmojiWidget;
    class LineWidget;
    class ActionButton;
    class FlatMenu;

    class InfoPlate : public QWidget
    {
        Q_OBJECT
    public:
        InfoPlate(QWidget* parent, int leftMargin);
        void setHeader(const QString& header);
        void setInfo(const QString& info, const QString& prefix = QString());
        void elideText(int width);
        void setAttachPhone(bool value);
        QString getInfoText() const;

    private Q_SLOTS:
        void menuRequested();
        void menu(QAction*);

    private:
        LabelEx* header_;
        TextEmojiWidget* info_;
        LabelEx* phoneInfo_;
        QString infoStr_;
        bool attachPhone_;
    };

    class ProfilePage : public SidebarPage
    {
        Q_OBJECT
    public:
        explicit ProfilePage(QWidget* parent);
        void initFor(const QString& aimId) override;

    protected:
        void paintEvent(QPaintEvent* e) override;
        void resizeEvent(QResizeEvent* e) override;
        void updateWidth() override;

    private Q_SLOTS:
        void chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>&);
        void removeFromIgnore();
        void contactChanged(const QString&);
        void contactRemoved(const QString&);
        void changed();
        void addClicked();
        void chatClicked();
        void callClicked();
        void videoClicked();
        void back();
        void rename();
        void ignoreList();
        void editClicked();
        void attachOld();
        void recvFlags(int);
        void saveClicked();
        void avatarChanged();
        void touchScrollStateChanged(QScroller::State);
        void copyLinkClicked();

    private:
        void init();
        void updateStatus();

    private:
        QString currentAimId_;
        QWidget* mainWidget_;
        ContactAvatarWidget* avatar_;
        CustomButton* backButton_;
        TextEditEx* name_;
        TextEditEx* nameEdit_;
        TextEditEx* descriptionEdit_;
        QWidget* buttonsMargin_;
        QWidget* ignoreWidget_;
        QWidget* buttonWidget_;
        LineWidget* Line_;
        QWidget* avatarBottomSpace_;
        QWidget* chatEditWidget_;
        LabelEx* ignoreDelete_;
        LabelEx* editLabel_;
        std::shared_ptr<Data::ChatInfo> info_;
        CustomButton* addButton_;
        CustomButton* chatButton_;
        CustomButton* callButton_;
        CustomButton* videoCall_button_;
        ActionButton* renameContact_;
        ActionButton* shareLink_;
        ActionButton* quiAndDelete;
        ActionButton* ignoreListButton;
        ActionButton* attachOldAcc;
        InfoPlate* uin_;
        InfoPlate* phone_;
        InfoPlate* firstName_;
        InfoPlate* lastName_;
        InfoPlate* nickName_;
        InfoPlate* birthday_;
        InfoPlate* city_;
        InfoPlate* country_;
        QWidget* nameMargin_;
        QPushButton* saveButton_;
        LabelEx* statusLabel_;
        QVBoxLayout* subBackButtonLayout_;
        QVBoxLayout* subAvatarLayout_;
        QVBoxLayout* mainBackButtonLayout_;
        QVBoxLayout* mainAvatarLayout_;
        QHBoxLayout* nameLayout_;
        QVBoxLayout* editLayout_;
        QVBoxLayout* subEditLayout_;
        QWidget* saveButtonMargin_;
        QWidget* saveButtonSpace_;
        bool connectOldVisible_;
        bool myInfo_;
    };
}
