#include "stdafx.h"

#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"

#include "KnownFileTypes.h"

namespace
{
	typedef std::map<QString, Themes::PixmapResourceId> Ext2IconMap;

    typedef QSet<QString> QStringSet;

	const Ext2IconMap& GetExt2IconMap();
}

namespace History
{
	const Themes::IThemePixmapSptr& GetIconByFilename(const QString &filename)
	{
		assert(!filename.isEmpty());

        const auto resId = GetIconIdByFilename(filename);
		return Themes::GetPixmap(resId);
	}

    Themes::PixmapResourceId GetIconIdByFilename(const QString &filename)
    {
        assert(!filename.isEmpty());

        QFileInfo fi(filename);

        const auto &map = GetExt2IconMap();

        const auto iter = map.find(fi.suffix());
        if (iter != map.end())
        {
            return iter->second;
        }

        return Themes::PixmapResourceId::FileSharingFileTypeIconUnknown;
    }
}

namespace
{
	const Ext2IconMap& GetExt2IconMap()
	{
        using namespace Themes;

		static Ext2IconMap map;

		if (!map.empty())
		{
            return map;
        }

        map.emplace(qsl("exe"), PixmapResourceId::FileSharingFileTypeIconExe);

        map.emplace(qsl("xls"),  PixmapResourceId::FileSharingFileTypeIconExcel);
        map.emplace(qsl("xlsx"), PixmapResourceId::FileSharingFileTypeIconExcel);
        map.emplace(qsl("csv"),  PixmapResourceId::FileSharingFileTypeIconExcel);

        map.emplace(qsl("html"), PixmapResourceId::FileSharingFileTypeIconHtml);
        map.emplace(qsl("htm"),  PixmapResourceId::FileSharingFileTypeIconHtml);

        map.emplace(qsl("png"),  PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace(qsl("jpg"),  PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace(qsl("jpeg"), PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace(qsl("bmp"),  PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace(qsl("gif"),  PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace(qsl("tif"),  PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace(qsl("tiff"), PixmapResourceId::FileSharingFileTypeIconImage);

        map.emplace(qsl("pdf"), PixmapResourceId::FileSharingFileTypeIconPdf);

        map.emplace(qsl("ppt"), PixmapResourceId::FileSharingFileTypeIconPpt);

        map.emplace(qsl("wav"),  PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace(qsl("mp3"),  PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace(qsl("ogg"),  PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace(qsl("flac"), PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace(qsl("aac"), PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace(qsl("m4a"), PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace(qsl("aiff"), PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace(qsl("ape"), PixmapResourceId::FileSharingFileTypeIconSound);

        map.emplace(qsl("log"), PixmapResourceId::FileSharingFileTypeIconTxt);
        map.emplace(qsl("txt"), PixmapResourceId::FileSharingFileTypeIconTxt);

        map.emplace(qsl("mp4"), PixmapResourceId::FileSharingFileTypeIconVideo);
        map.emplace(qsl("avi"), PixmapResourceId::FileSharingFileTypeIconVideo);
        map.emplace(qsl("mkv"), PixmapResourceId::FileSharingFileTypeIconVideo);
        map.emplace(qsl("wmv"), PixmapResourceId::FileSharingFileTypeIconVideo);
        map.emplace(qsl("flv"), PixmapResourceId::FileSharingFileTypeIconVideo);
        map.emplace(qsl("webm"), PixmapResourceId::FileSharingFileTypeIconVideo);

        map.emplace(qsl("doc"),  PixmapResourceId::FileSharingFileTypeIconWord);
        map.emplace(qsl("docx"), PixmapResourceId::FileSharingFileTypeIconWord);
        map.emplace(qsl("rtf"),  PixmapResourceId::FileSharingFileTypeIconWord);

        map.emplace(qsl("zip"), PixmapResourceId::FileSharingFileTypeIconZip);
        map.emplace(qsl("rar"), PixmapResourceId::FileSharingFileTypeIconZip);

		return map;
	}

}