#pragma once

namespace Ui
{
    class CustomButton;
    class LabelEx;

    class LineWidget : public QWidget
    {
        Q_OBJECT
    public:
        LineWidget(QWidget* parent, int leftMargin, int topMargin, int rightMargin, int bottomMargin);
        void setLineWidth(int width);
    private:
        QWidget* line_;
    };


    class ActionButton : public QWidget
    {
        Q_OBJECT
Q_SIGNALS:
        void clicked();

    public:
        ActionButton(QWidget* parent, const QString& image, const QString& text, int height, int leftMargin, int textOffset);

        void setImage(const QString& image);
        void setText(const QString& text);
        void setColor(const QString& color);
        void setFont(const QFont& font);
        void setLink(const QString& text, const QColor& color);

    protected:
        void paintEvent(QPaintEvent*) override;
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void resizeEvent(QResizeEvent *) override;

    private:
        void elideLink();

    private:
        bool Hovered_;
        int Height_;
        CustomButton* Button_;
        LabelEx* Link_;
        QString LinkText_;
    };


    class ClickedWidget : public QWidget
    {
        Q_OBJECT
Q_SIGNALS:
        void clicked();

    public:
        ClickedWidget(QWidget* parent);
        void setEnabled(bool value);

    protected:
        void paintEvent(QPaintEvent*) override;
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void mouseReleaseEvent(QMouseEvent *) override;

    private:
        bool Hovered_;
        bool Enabled_;
    };
}
