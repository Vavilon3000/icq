#ifndef __MENTIONS_ME_H_
#define __MENTIONS_ME_H_

namespace core
{
    namespace archive
    {
        class archive_index;
        class contact_archive;
        class history_message;
        class storage;

        typedef std::vector<std::shared_ptr<history_message>> history_block;

        class mentions_me
        {
            history_block messages_;

            std::unique_ptr<storage> storage_;

        public:

            mentions_me(const std::wstring& _file_name);
            virtual ~mentions_me();

            void serialize(core::tools::binary_stream& _data) const;
            bool unserialize(core::tools::binary_stream& _data);

            void add(std::shared_ptr<archive::history_message> _message);
        };

        
    }
}

#endif//__MENTIONS_ME_H_
