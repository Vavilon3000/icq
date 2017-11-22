#include "stdafx.h"

#include "translator_base.h"

namespace translate
{
    void translator_base::init()
    {
        QString lang = getLang();

        QLocale::setDefault(QLocale(lang));
        static QTranslator translator;
        translator.load(lang, qsl(":/translations"));

        QApplication::installTranslator(&translator);
    }

    QString translator_base::formatDate(const QDate& target, bool currentYear) const
    {
        QString format = currentYear ? getCurrentYearDateFormat() : getOtherYearsDateFormat();
        QString result = QLocale().toString(target, format);
        if (getLang() == ql1s("pt"))
            result = result.arg(qsl("de"));

        return result;
    }

    QString translator_base::getNumberString(int number, const QString& one, const QString& two, const QString& five, const QString& twentyOne) const
    {
        if (number == 1)
            return one;

        QString result;
        int strCase = number % 10;
        if (strCase == 1 && number % 100 != 11)
        {
            result = twentyOne;
        }
        else if (strCase > 4 || strCase == 0 || (number % 100 > 10 && number % 100 < 20))
        {
            result = five;
        }
        else
        {
            result = two;
        }

        return result;
    }

    QString translator_base::getCurrentYearDateFormat() const
    {
        const QString lang = getLang();
        if (lang == ql1s("ru"))
            return qsl("d MMM");
        else if (lang == ql1s("de"))
            return qsl("d. MMM");
        else if (lang == ql1s("pt"))
            return qsl("d %1 MMMM");
        else if (lang == ql1s("uk"))
            return qsl("d MMM");
        else if (lang == ql1s("cs"))
            return qsl("d. MMM");
        else if (lang == ql1s("fr"))
            return qsl("Le d MMM");

        return qsl("MMM d");
    }

    QString translator_base::getOtherYearsDateFormat() const
    {
        const QString lang = getLang();
        if (lang == ql1s("ru"))
            return qsl("d MMM yyyy");
        else if (lang == ql1s("de"))
            return qsl("d. MMM yyyy");
        else if (lang == ql1s("pt"))
            return qsl("d %1 MMMM %1 yyyy");
        else if (lang == ql1s("uk"))
            return qsl("d MMM yyyy");
        else if (lang == ql1s("cs"))
            return qsl("d. MMM yyyy");
        else if (lang == ql1s("fr"))
            return qsl("Le d MMM yyyy");

        return qsl("MMM d, yyyy");
    }

    const QList<QString> &translator_base::getLanguages() const
    {
        static QList<QString> clist;
        if (clist.isEmpty())
        {
            clist.push_back(qsl("ru"));
            clist.push_back(qsl("en"));
            clist.push_back(qsl("uk"));
            clist.push_back(qsl("de"));
            clist.push_back(qsl("pt"));
            clist.push_back(qsl("cs"));
            clist.push_back(qsl("fr"));
        }
        return clist;
    }

    QString translator_base::getLang() const
    {
        QString lang;

        const QString localLang = QLocale::system().name();
        if (localLang.isEmpty())
            lang = qsl("en");
        else
            lang = localLang.left(2);

        if (lang != ql1s("ru") && lang != ql1s("en") && lang != ql1s("uk") && lang != ql1s("de") && lang != ql1s("pt") && lang != ql1s("cs") && lang != ql1s("fr"))
            lang = qsl("en");

        return lang;
    }
}