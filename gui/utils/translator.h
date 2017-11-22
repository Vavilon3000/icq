#pragma once

#include "../../gui.shared/translator_base.h"

namespace Utils
{
	class Translator : public translate::translator_base
	{
	public:
		virtual void init() override;
		QString getCurrentPhoneCode() const;
        QString getCurrentLang() const;

        void updateLocale();

	private:

		virtual QString getLang() const override;
	};

	Translator* GetTranslator();
}