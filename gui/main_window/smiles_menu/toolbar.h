#pragma once
#include "../../controls/CustomButton.h"
namespace Ui
{
    class smiles_Widget;
    class CustomButton;

    namespace Smiles
    {
        class AttachedView
        {
            QWidget* View_;
            QWidget* ViewParent_;

        public:

            AttachedView(QWidget* _view, QWidget* _viewParent = nullptr);

            QWidget* getView();
            QWidget* getViewParent();
        };


        class AddButton : public QWidget
        {
            Q_OBJECT

            Q_SIGNALS :

            void clicked();

        public:

            AddButton(QWidget* _parent);

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void resizeEvent(QResizeEvent * _e) override;
        };


        //////////////////////////////////////////////////////////////////////////
        // TabButton
        //////////////////////////////////////////////////////////////////////////
        class TabButton : public CustomButton
        {
            Q_OBJECT

            void Init();

            const QString resource_;

            AttachedView AttachedView_;

            bool Fixed_;

        public:

            TabButton(QWidget* _parent);
            TabButton(QWidget* _parent, const QString& _resource);

            void AttachView(const AttachedView& _view);
            const AttachedView& GetAttachedView() const;

            void setFixed(const bool _isFixed);
            bool isFixed() const;

            ~TabButton();
        };

        enum buttons_align
        {
            center = 1,
            left = 2,
            right = 3
        };




        //////////////////////////////////////////////////////////////////////////
        // Toolbar
        //////////////////////////////////////////////////////////////////////////
        class Toolbar : public QFrame
        {
            Q_OBJECT

        private:

            enum direction
            {
                left = 0,
                right = 1
            };

            buttons_align align_;
            QHBoxLayout* horLayout_;
            std::list<TabButton*> buttons_;
            QScrollArea* viewArea_;

            AddButton* buttonStore_;

            QPropertyAnimation* AnimScroll_;

            void addButton(TabButton* _button);
            void initScroll();
            void scrollStep(direction _direction);

        private Q_SLOTS:
            void touchScrollStateChanged(QScroller::State);
            void buttonStoreClick();

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void resizeEvent(QResizeEvent * _e) override;
            virtual void wheelEvent(QWheelEvent* _e) override;
        public:

            Toolbar(QWidget* _parent, buttons_align _align);
            ~Toolbar();

            void Clear(const bool _delFixed = false);

            TabButton* addButton(const QString& _resource);
            TabButton* addButton(const QPixmap& _icon);

            void scrollToButton(TabButton* _button);

            const std::list<TabButton*>& GetButtons() const;

            void addButtonStore();
        };
    }
}