#pragma once

namespace Ui
{
    class SemitransparentWindowAnimated;

    enum SidebarPages
    {
        menu_page = 0,
        profile_page = 1,
        all_members = 2,
    };

    class SidebarWidth
    {
    public:
        SidebarWidth() : width_(0) {}
        virtual ~SidebarWidth() {}
        virtual void setSidebarWidth(int width) { width_ = width; updateWidth(); }

    protected:
        virtual void updateWidth() = 0;

    protected:
        int width_;
    };

    class SidebarPage : public QWidget, public SidebarWidth
    {
    public:
        SidebarPage(QWidget* parent) : QWidget(parent) {}
        virtual ~SidebarPage() {}
        virtual void setPrev(const QString& aimId) { prevAimId_ = aimId; }

        virtual void initFor(const QString& aimId) = 0;

    protected:
        QString prevAimId_;
    };

    //////////////////////////////////////////////////////////////////////////

    class Sidebar : public QStackedWidget, public SidebarWidth
    {
        Q_OBJECT

    public Q_SLOTS:
        void showAnimated();
        void hideAnimated();

    public:
        Sidebar(QWidget* _parent, bool _isEmbedded = false);
        void preparePage(const QString& aimId, SidebarPages page);
        void setSidebarWidth(int width) override;
        void showAllMembers();
        int currentPage() const;
        QString currentAimId() const;
        void updateSize();

    protected:
        void updateWidth() override;

    private Q_SLOTS:
        void contactRemoved(QString);
        void onAnimationFinished();

    private:
        QMap<int, SidebarPage*> pages_;
        QString currentAimId_;
        SemitransparentWindowAnimated* semiWindow_;
        QPropertyAnimation* animSidebar_;
        int anim_;
        bool embedded_;

    private:
        Q_PROPERTY(int anim READ getAnim WRITE setAnim)
        void setAnim(int _val);
        int getAnim() const;
    };
}
