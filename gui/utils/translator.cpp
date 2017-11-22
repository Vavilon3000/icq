#include "stdafx.h"

#include "../cache/countries.h"
#include "../gui_settings.h"
#include "../utils/gui_coll_helper.h"
#include "../core_dispatcher.h"

#ifdef __APPLE__
#include "macos/mac_support.h"
#endif

namespace Utils
{
    void Translator::init()
    {
        QString lang = getLang();
        Ui::get_gui_settings()->set_value<QString>(settings_language, lang);

        QLocale::setDefault(QLocale(lang));
        static QTranslator translator;
        translator.load(lang, qsl(":/translations"));
        QApplication::installTranslator(&translator);
        updateLocale();
    }

    void Translator::updateLocale()
    {
        QLocale locale(getLang());
        QString localeStr = locale.name();
        localeStr.replace(ql1c('_'), ql1c('-'));

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("locale", std::move(localeStr).toLower());
        Ui::GetDispatcher()->post_message_to_core(qsl("set_locale"), collection.get());
    }


    QString Translator::getCurrentPhoneCode() const
    {
        const Ui::countries::countries_list& cntrs = Ui::countries::get();

        QString result;
        result += ql1c('+');

#ifdef __APPLE__
        QString searchedCounry = MacSupport::currentRegion().right(2).toLower();
#else
        QString searchedCounry = QLocale::system().name().right(2).toLower();
#endif

        for (const auto& country : cntrs)
        {
            if (country.iso_code_ == searchedCounry)
            {
                result += QString::number(country.phone_code_);
                break;
            }
        }

        return result;
    }

    QString Translator::getCurrentLang() const
    {
        return getLang();
    }

    QString Translator::getLang() const
    {
        return Ui::get_gui_settings()->get_value<QString>(settings_language, translate::translator_base::getLang());
    }

    Translator* GetTranslator()
    {
        static auto translator = std::make_unique<Translator>();
        return translator.get();
    }
}
