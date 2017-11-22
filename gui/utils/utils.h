#pragma once

#include "../../corelib/enumerations.h"
#include "../types/message.h"

class QApplication;

#ifdef _WIN32
QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);
#endif //_WIN32

namespace Ui
{
    class GeneralDialog;
}

namespace Utils
{
    template <typename... Args>
    struct QNonConstOverload
    {
        template <typename R, typename T>
        Q_DECL_CONSTEXPR auto operator()(R(T::*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
        {
            return ptr;
        }

        template <typename R, typename T>
        static Q_DECL_CONSTEXPR auto of(R(T::*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
        {
            return ptr;
        }
    };

    template <typename... Args>
    struct QConstOverload
    {
        template <typename R, typename T>
        Q_DECL_CONSTEXPR auto operator()(R(T::*ptr)(Args...) const) const Q_DECL_NOTHROW -> decltype(ptr)
        {
            return ptr;
        }

        template <typename R, typename T>
        static Q_DECL_CONSTEXPR auto of(R(T::*ptr)(Args...) const) Q_DECL_NOTHROW -> decltype(ptr)
        {
            return ptr;
        }
    };

    template <typename... Args>
    struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...>
    {
        using QConstOverload<Args...>::of;
        using QConstOverload<Args...>::operator();
        using QNonConstOverload<Args...>::of;
        using QNonConstOverload<Args...>::operator();

        template <typename R>
        Q_DECL_CONSTEXPR auto operator()(R(*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
        {
            return ptr;
        }

        template <typename R>
        static Q_DECL_CONSTEXPR auto of(R(*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
        {
            return ptr;
        }
    };

#if defined(__cpp_variable_templates) && __cpp_variable_templates >= 201304 // C++14
    template <typename... Args> Q_CONSTEXPR Q_DECL_UNUSED QOverload<Args...> qOverload = {};
    template <typename... Args> Q_CONSTEXPR Q_DECL_UNUSED QConstOverload<Args...> qConstOverload = {};
    template <typename... Args> Q_CONSTEXPR Q_DECL_UNUSED QNonConstOverload<Args...> qNonConstOverload = {};
#endif


    class ShadowWidgetEventFilter : public QObject
    {
        Q_OBJECT

    public:
        ShadowWidgetEventFilter(int _shadowWidth);

    protected:
        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        void setGradientColor(QGradient& _gradient, bool _isActive);

    private:
        int ShadowWidth_;
    };

    QString getCountryNameByCode(const QString& _iso_code);
    QMap<QString, QString> getCountryCodes();

    QString ScaleStyle(const QString& _style, double _scale);

    void ApplyStyle(QWidget* _widget, const QString& _style);
    void ApplyPropertyParameter(QWidget* _widget, const char* _property, QVariant _parameter);

    QString LoadStyle(const QString& _qssFile);

    QPixmap getDefaultAvatar(const QString& _uin, const QString& _displayName, const int _sizePx);

    std::vector<QStringList> GetPossibleStrings(const QString& _text, unsigned& _count);

    QPixmap roundImage(const QPixmap& _img, const QString& _state, bool _isDefault, bool _miniIcons);

    void addShadowToWidget(QWidget* _target);
    void addShadowToWindow(QWidget* _target, bool _enabled = true);

    void grabTouchWidget(QWidget* _target, bool _topWidget = false);

    void removeLineBreaks(QString& _source);

    bool isValidEmailAddress(const QString& _email);

    bool foregroundWndIsFullscreened();

    QColor getSelectionColor();
    QString rgbaStringFromColor(const QColor& _color);

    double fscale_value(const double _px);
    int scale_value(const int _px);
    QSize scale_value(const QSize _px);
    QSizeF scale_value(const QSizeF _px);
    QRect scale_value(const QRect _px);
    int unscale_value(int _px);
    QSize unscale_value(const QSize& _px);
    int scale_bitmap(const int _px);
    int unscale_bitmap(const int _px);
    QSize scale_bitmap(const QSize& _px);
    QSize unscale_bitmap(const QSize& _px);
    QRect scale_bitmap(const QRect& _px);
    int scale_bitmap_with_value(const int _px);
    QSize scale_bitmap_with_value(const QSize& _px);
    QRect scale_bitmap_with_value(const QRect& _px);

    template <typename _T>
    void check_pixel_ratio(_T& _image);

    QString	parse_image_name(const QString& _imageName);
    bool	is_mac_retina();
    void	set_mac_retina(bool _val);
    double	getScaleCoefficient();
    void	setScaleCoefficient(double _coefficient);
    double	getBasicScaleCoefficient();
    void	initBasicScaleCoefficient(double _coefficient);

    void groupTaskbarIcon(bool _enabled);

    bool isStartOnStartup();
    void setStartOnStartup(bool _start);

#ifdef _WIN32
    HWND createFakeParentWindow();
#endif //WIN32

    uint getInputMaximumChars();

    int calcAge(const QDateTime& _birthdate);

    void drawText(QPainter & painter, const QPointF & point, int flags,
        const QString & text, QRectF * boundingRect = nullptr);

    QString DefaultDownloadsPath();
    QString UserDownloadsPath();

    bool is_image_extension(const QString& _ext);
    bool is_image_extension_not_gif(const QString& _ext);
    bool is_video_extension(const QString& _ext);

    void copyFileToClipboard(const QString& _path);

    void saveAs(const QString& _inputFilename, std::function<void (const QString& _filename, const QString& _directory)> _callback, std::function<void ()> _cancel_callback = std::function<void ()>(), bool asSheet = true /* for OSX only */);

    using SendKeysIndex = std::vector<std::pair<QString, Ui::KeyToSendMessage>>;

    const SendKeysIndex& getSendKeysIndex();


    void post_stats_with_settings();
    QRect GetMainRect();
    QPoint GetMainWindowCenter();
    QRect GetWindowRect(QWidget* window);

    void UpdateProfile(const std::vector<std::pair<std::string, QString>>& _fields);

    QString getItemSafe(const std::vector<QString>& _values, size_t _selected, const QString& _default);

    Ui::GeneralDialog *NameEditorDialog(
        QWidget* _parent,
        const QString& _chatName,
        const QString& _buttonText,
        const QString& _headerText,
        Out QString& resultChatName,
        bool acceptEnter = true);

    bool NameEditor(
        QWidget* _parent,
        const QString& _chatName,
        const QString& _buttonText,
        const QString& _headerText,
        Out QString& resultChatName,
        bool acceptEnter = true);

    bool GetConfirmationWithTwoButtons(const QString& _buttonLeft, const QString& _buttonRight,
        const QString& _messageText, const QString& _labelText, QWidget* _parent, QWidget* _mainWindow = nullptr);

    bool GetErrorWithTwoButtons(const QString& _buttonLeftText, const QString& _buttonRightText,
        const QString& _messageText, const QString& _labelText, const QString& _errorText, QWidget* _parent);

    struct ProxySettings
    {
        const static int invalidPort = -1;

        core::proxy_types type_;
        QString username_;
        bool needAuth_;
        QString password_;
        QString proxyServer_;
        int port_;

        ProxySettings(core::proxy_types _type, QString _username, QString _password,
            QString _proxy, int _port, bool _needAuth);

        ProxySettings();

        void postToCore();
    };

    ProxySettings* get_proxy_settings();

    bool loadPixmap(const QString& _path, Out QPixmap& _pixmap);

    bool loadPixmap(const QByteArray& _data, Out QPixmap& _pixmap);

    bool dragUrl(QWidget* _parent, const QPixmap& _preview, const QString& _url);

    QString extractUinFromIcqLink(const QString &_uri);

    class StatsSender : public QObject
    {
        Q_OBJECT
    public :
        StatsSender();

    public Q_SLOTS:
        void recvGuiSettings() { guiSettingsReceived_ = true; trySendStats(); }
        void recvThemeSettings() { themeSettingsReceived_ = true; trySendStats(); }

    public:
        void trySendStats() const;

    private:
        bool guiSettingsReceived_;
        bool themeSettingsReceived_;
    };

    StatsSender* getStatsSender();

    bool haveText(const QMimeData *);

    QString normalizeLink(const QString& _link);

    const wchar_t* get_crossprocess_mutex_name();
    const char* get_crossprocess_pipe_name();

    QHBoxLayout* emptyHLayout(QWidget* parent = nullptr);
    QVBoxLayout* emptyVLayout(QWidget* parent = nullptr);

    QString getProductName();
    QString getInstallerName();

    void openMailBox(const QString& email, const QString& mrimKey, const QString& mailId);
    void openAgentUrl(
        const QString& _url,
        const QString& _fail_url,
        const QString& _email,
        const QString& _mrimKey);

    QString getUnreadsBadgeStr(int _unreads);
    void drawUnreads(QPainter *p, const QFont &font, QColor bgColor, QColor textColor, QColor borderColor, int unreads, int balloonSize, int x, int y);
    QPoint getUnreadsSize(QPainter *p, const QFont &font, bool bBorder, int unreads, int balloonSize);

    QImage iconWithCounter(int size, int count, QColor bg, QColor fg, QImage back = QImage());

    void openUrl(const QString& _url);

    QString convertMentions(const QString& _source, const Data::MentionMap& _mentions);

    void openDialogOrProfile(const QString& _contact, const QString& _chat = QString());
}

