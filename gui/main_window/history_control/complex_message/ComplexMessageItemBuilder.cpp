#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "../../../utils/utils.h"

#include "../KnownFileTypes.h"

#include "ComplexMessageItem.h"
#include "FileSharingBlock.h"
#include "FileSharingUtils.h"
#include "ImagePreviewBlock.h"
#include "LinkPreviewBlock.h"
#include "PttBlock.h"
#include "TextBlock.h"
#include "QuoteBlock.h"
#include "StickerBlock.h"
#include "TextChunk.h"

#include "ComplexMessageItemBuilder.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

    namespace ComplexMessageItemBuilder
{
    std::unique_ptr<ComplexMessageItem> makeComplexItem(
        QWidget *_parent,
        const int64_t _id,
        const QDate _date,
        const int64_t _prev,
        const QString &_text,
        const QString &_chatAimid,
        const QString &_senderAimid,
        const QString &_senderFriendly,
        const QVector<Data::Quote>& _quotes,
        const Data::MentionMap& _mentions,
        HistoryControl::StickerInfoSptr _sticker,
        const bool _isOutgoing,
        const bool _isNotAuth)
    {
        assert(_id >= -1);
        assert(!_senderAimid.isEmpty());
        assert(!_senderFriendly.isEmpty());
        assert(_date.isValid());

        auto complexItem = std::make_unique<ComplexMessage::ComplexMessageItem>(
            _parent,
            _id,
            _date,
            _chatAimid,
            _senderAimid,
            _senderFriendly,
            _sticker ? QT_TRANSLATE_NOOP("contact_list", "Sticker") : _text,
            _mentions,
            _isOutgoing);

        std::list<TextChunk> chunks;

        std::unique_ptr<TextChunk> snippet;
        int snippetsCount = 0;
        bool removeSnippetOnFail = false;

        int i = 0;
        for (auto quote : _quotes)
        {
            ++i;
            quote.id_ = i;
            const auto text = quote.text_;
            const auto isFirstQuote = (i == 1);
            const auto isLastQuote = (i == _quotes.size());

            if (quote.isSticker())
            {
                TextChunk chunk(TextChunk::Type::Sticker, text, QString(), -1);
                chunk.Sticker_ = HistoryControl::StickerInfo::Make(quote.setId_, quote.stickerId_);
                chunk.Quote_ = std::move(quote);
                chunk.Quote_.isFirstQuote_ = isFirstQuote;
                chunk.Quote_.isLastQuote_ = isLastQuote;
                chunks.emplace_back(std::move(chunk));
                continue;
            }

            if (text.startsWith(ql1c('>')))
            {
                TextChunk chunk(TextChunk::Type::Text, text, QString(), -1);
                chunk.Quote_ = std::move(quote);
                chunk.Quote_.mentions_ = _mentions;
                chunk.Quote_.isFirstQuote_ = isFirstQuote;
                chunk.Quote_.isLastQuote_ = isLastQuote;
                chunks.emplace_back(std::move(chunk));
                continue;
            }

            ChunkIterator it(text);
            while (it.hasNext())
            {
                auto chunk = it.current();
                chunk.Quote_ = quote;
                chunk.Quote_.mentions_ = _mentions;
                chunk.Quote_.isFirstQuote_ = isFirstQuote;
                chunk.Quote_.isLastQuote_ = isLastQuote;
                chunks.emplace_back(std::move(chunk));
                it.next();
            }
        }

        const bool hide_links = (_isNotAuth && !_isOutgoing);

        if (_sticker)
        {
            TextChunk chunk(TextChunk::Type::Sticker, _text, QString(), -1);
            chunk.Sticker_ = _sticker;
            chunks.emplace_back(std::move(chunk));
        }
        else
        {
            ChunkIterator it(_text);
            while (it.hasNext())
            {
                auto chunk = it.current(!hide_links);

                if (chunk.Type_ == TextChunk::Type::GenericLink)
                {
                    ++snippetsCount;
                    if (snippetsCount == 1)
                        snippet = std::make_unique<TextChunk>(chunk);

                    // if snippet is single in message or if it last, don't append link
                    it.next();
                    if (it.hasNext() || snippetsCount > 1)
                    {
                        removeSnippetOnFail = true;
                        chunks.emplace_back(std::move(chunk));
                    }

                    continue;
                }
                else
                {
                    chunks.emplace_back(std::move(chunk));
                }

                it.next();
            }
        }

        for (auto iter = chunks.begin(); iter != chunks.end(); )
        {
            const auto next = std::next(iter);

            if (next == chunks.end())
            {
                break;
            }

            if (iter->Quote_.isEmpty())
            {
                auto merged = iter->mergeWith(*next);

                if (merged.Type_ != TextChunk::Type::Undefined)
                {
                    chunks.erase(iter);
                    *next = std::move(merged);
                }
            }
            iter = next;
        }

        IItemBlocksVec items;
        items.reserve(chunks.size());

        QuoteBlock* prevQuote = nullptr;
        std::vector<QuoteBlock*> quoteBlocks;

        int count = 0;
        for (const auto& chunk : chunks)
        {
            ++count;

            const auto& chunkText = chunk.text_;

            GenericBlock *block = nullptr;
            switch(chunk.Type_)
            {
            case TextChunk::Type::Text:
                block = new TextBlock(
                    complexItem.get(),
                    chunkText);
                break;

            case TextChunk::Type::GenericLink:
                block = new TextBlock(
                    complexItem.get(),
                    chunkText,
                    hide_links);
                break;

            case TextChunk::Type::ImageLink:
                if (hide_links)
                {
                    block = new TextBlock(
                        complexItem.get(),
                        chunkText,
                        true);
                }
                else
                {
                    block = new ImagePreviewBlock(
                        complexItem.get(),
                        _chatAimid,
                        chunkText,
                        chunk.ImageType_);
                }
                break;

            case TextChunk::Type::FileSharingImage:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::image);
                break;

            case TextChunk::Type::FileSharingGif:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::gif);
                break;

            case TextChunk::Type::FileSharingVideo:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::video);
                break;

            case TextChunk::Type::FileSharingPtt:
                block = new PttBlock(complexItem.get(), chunkText, chunk.DurationSec_, _id, _prev);
                break;

            case TextChunk::Type::FileSharingGeneral:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::undefined);
                break;

            case  TextChunk::Type::Sticker:
                block = new StickerBlock(complexItem.get(), chunk.Sticker_);
                break;

            case TextChunk::Type::Junk:
                break;

            default:
                assert(!"unexpected chunk type");
                break;
            }

            if (block)
            {
                if (!chunk.Quote_.isEmpty())
                {
                    if (prevQuote && prevQuote->getQuote().id_ == chunk.Quote_.id_)
                    {
                        prevQuote->addBlock(block);
                    }
                    else
                    {
                        auto quoteBlock = new QuoteBlock(complexItem.get(), chunk.Quote_);
                        quoteBlock->addBlock(block);
                        items.emplace_back(quoteBlock);
                        quoteBlocks.push_back(quoteBlock);
                        prevQuote = quoteBlock;

                        if (!chunk.Quote_.chatName_.isEmpty() || !chunk.Quote_.isForward_)
                            quoteBlock->createQuoteHover(complexItem.get());
                    }
                }
                else
                {
                    for (auto q : quoteBlocks)
                    {
                        q->setReplyBlock(block);
                    }
                    items.emplace_back(block);
                }
            }
        }

        // create snippet
        if (snippetsCount == 1)
        {
            items.emplace_back(
                new LinkPreviewBlock(
                complexItem.get(),
                snippet->text_,
                removeSnippetOnFail));
        }


        if (!quoteBlocks.empty())
        {
            QString sourceText;
            for (auto item : items)
            {
                sourceText += item->getSourceText();
            }
            complexItem->setSourceText(std::move(sourceText));
        }

        int index = 0;
        for (auto& val : quoteBlocks)
        {
            val->setMessagesCountAndIndex(count, index++);
        }

        complexItem->setItems(std::move(items));

        return complexItem;
    }
}

UI_COMPLEX_MESSAGE_NS_END
