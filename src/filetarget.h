
namespace xdccd
{


class FileTarget : AbstractTarget
{
    public:
        FileTargetifile_id_t id, const boost::filesystem::path &base_path, const std::string &filename, std::uintmax_t size);
        void open();
        void close();
        void write(const char* data, std::streamsize len);

};

typedef std::shared_ptr<DCCFile> DCCFilePtr;

}
