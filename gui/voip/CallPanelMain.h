#ifndef __CALL_PANEL_H__
#define __CALL_PANEL_H__

#include "AvatarContainerWidget.h"
#include "NameAndStatusWidget.h"
#include "PushButton_t.h"
#include "secureCallWnd.h"
#include "VideoPanel.h"
#include "WindowHeaderFormat.h"

namespace voip_manager
{
    struct CipherState;
    struct ContactEx;
    struct Contact;
}

class QLabel;
namespace Ui
{
    class VolumeGroup;

    class SliderEx : public QWidget
    {
        Q_OBJECT

    private Q_SLOTS:
        void onVolumeChanged(int);
        void onVolumeReleased();
        void onIconClicked();

    Q_SIGNALS:
        void onSliderReleased();
        void onIconClick();
        void onSliderValueChanged(int);

    public:
        SliderEx(QWidget* _parent);
        ~SliderEx();

        void setValue(int);
        void setEnabled(bool);

        void setIconForState(const PushButton_t::eButtonState _state, const std::string& _image);
        void setIconSize(const int _w, const int _h);

        void setPropertyForIcon(const char* _name, bool _val);
        void setPropertyForSlider(const char* _name, bool _val);

    private:
        QHBoxLayout  *horizontalLayout_;
        PushButton_t *sliderIcon_;
        QSlider      *slider_;
    };

    class QPushButtonEx;

    class CallPanelMainEx : public QWidget
    {
        Q_OBJECT
        public:
            struct CallPanelMainFormat
            {
                eVideoPanelHeaderItems bottomPartFormat;
                unsigned bottomPartHeight;
            };

        private Q_SLOTS:

            void onClickGoChat();
            void onCameraTurn();
            void onStopCall();
            void onMicTurn();
            void onSoundTurn();
            void addUserToVideoCall();
            void onScreenSharing();

            void onVoipCallNameChanged(const voip_manager::ContactsList&);
            void onVoipMediaLocalVideo(bool _enabled);
            void onVoipMediaLocalAudio(bool _enabled);
            void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
            void onVoipCallTimeChanged(unsigned _secElapsed, bool _haveCall);
            void onVoipVideoDeviceSelected(const voip_proxy::device_desc& desc);

            void onVoipCallOutAccepted(const voip_manager::ContactEx& _contactEx);
            void onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contactEx);
            void onVoipCallConnected(const voip_manager::ContactEx& _contactEx);

        Q_SIGNALS:

            void onClickOpenChat(const std::string& _contact);
            void onBackToVideo();

        protected:
            void hideEvent(QHideEvent* _e) override;
            void moveEvent(QMoveEvent* _e) override;
            void resizeEvent(QResizeEvent* _e) override;
            void enterEvent(QEvent* _e) override;
            void mouseReleaseEvent(QMouseEvent *event) override;

        public:
            CallPanelMainEx(QWidget* _parent, const CallPanelMainFormat& _panelFormat);
            virtual ~CallPanelMainEx();

        private:
            template<typename ButtonType>
            ButtonType* addButton(QWidget& _parentWidget, const QString& _propertyName, const char* _slot, bool _bDefaultCursor = false, bool rightAlignment = false);

            VolumeGroup* addVolumeGroup(QWidget& _parentWidget, bool rightAlignment, int verticalSize);
            void updateVideoDeviceButtonsState();

        private:
            std::string activeContact_;
            const CallPanelMainFormat format_;
            QWidget* rootWidget_;
            TextEmojiWidget* nameLabel_;
            VolumeControl vVolControl_;
            VolumeControl hVolControl_;
            bool secureCallEnabled_;
            VolumeGroup* volumeGroup;
            TextEmojiWidget* timeLabel_;
            bool isScreenSharingEnabled_;
            bool  localVideoEnabled_;
            bool isCameraEnabled_;

        private:
            QPushButton* buttonMaximize_;
            QPushButton* buttonLocalCamera_;
            QPushButton* buttonLocalMic_;
            QPushButton* buttonAddUser_;
            QPushButton* buttonStopCall_;
            QPushButton* buttonGoChat_;
            QPushButton* screenSharing_;
    };
}

#endif//__CALL_PANEL_H__
