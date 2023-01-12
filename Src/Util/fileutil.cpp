#include "fileutil.h"
#include <boost/filesystem/directory.hpp>
#include <boost/system/detail/error_code.hpp>

#include <fstream>
#include <ios>
#include <sstream>


namespace util { namespace file {

bool fileExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat)
{
    boost::system::error_code err;
    auto file_status = boost::filesystem::status(path, err);

    if (stat)
    {
        *stat = file_status;
    }

    if (err || !boost::filesystem::exists(file_status) || !boost::filesystem::is_regular_file(file_status))
    {
        return false;
    }

    return true;
}

bool dirExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat)
{
    boost::system::error_code err;
    auto file_status = boost::filesystem::status(path, err);

    if (stat)
    {
        *stat = file_status;
    }

    if (err || !boost::filesystem::exists(file_status) || !boost::filesystem::is_directory(file_status))
    {
        return false;
    }
    return true;
}

bool fileOrDirExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat)
{
    boost::system::error_code err;
    auto file_status = boost::filesystem::status(path, err);

    if (stat)
    {
        *stat = file_status;
    }

    if (err || !boost::filesystem::exists(file_status))
    {
        return false;
    }
    return true;
}


bool canRead(const boost::filesystem::path& path)
{
    boost::filesystem::file_status stat;
    if (!fileExist(path, &stat))
    {
        return false;
    }

    boost::filesystem::perms perms = stat.permissions();
    if ((perms & boost::filesystem::perms::owner_read) == boost::filesystem::perms::no_perms ||
        (perms & boost::filesystem::perms::group_read) == boost::filesystem::perms::no_perms ||
        (perms & boost::filesystem::perms::others_read) == boost::filesystem::perms::no_perms
    )
    {
        return false;
    }

    return true;
}

bool canWrite(const boost::filesystem::path& path)
{
    boost::filesystem::file_status stat;
    if (!fileExist(path, &stat))
    {
        return true;
    }

    boost::filesystem::perms perms = stat.permissions();
    if ((perms & boost::filesystem::perms::owner_write) == boost::filesystem::perms::no_perms ||
        (perms & boost::filesystem::perms::group_write) == boost::filesystem::perms::no_perms ||
        (perms & boost::filesystem::perms::others_write) == boost::filesystem::perms::no_perms
    )
    {
        return false;
    }

    return true;
}

bool makeFileReadable(const boost::filesystem::path& path)
{
    if (canRead(path))
    {
        return true;
    }
    boost::system::error_code err;
    boost::filesystem::permissions(
        path,
        boost::filesystem::perms::owner_read |
        boost::filesystem::perms::group_read |
        boost::filesystem::perms::others_read |
        boost::filesystem::perms::add_perms, err
    );
    return !err;
}

bool makeFileWritable(const boost::filesystem::path& path)
{
    if (canWrite(path))
    {
        return true;
    }

    boost::system::error_code err;
    boost::filesystem::permissions(
        path,
        boost::filesystem::perms::owner_write |
        boost::filesystem::perms::group_write |
        boost::filesystem::perms::others_write |
        boost::filesystem::perms::add_perms,
        err
    );
    return !err;
}

bool readFile(const boost::filesystem::path& path, std::string &content)
{
    if (!fileExist(path))
    {
        return false;
    }

    if (!canRead(path) && !makeFileReadable(path))
    {
        return false;
    }
    std::fstream file;
    file.open(path.string(), std::ios_base::in);
    if (file.is_open())
    {
        std::stringstream ss;
        ss << file.rdbuf();
        content = ss.str();
        file.close();
        return true;
    }
    return false;
}

bool readFile(const boost::filesystem::path& path, std::vector<char> &content)
{
    if (!fileExist(path))
    {
        return false;
    }

    if (!canRead(path) && !makeFileReadable(path))
    {
        return false;
    }

    std::fstream file;
    file.open(path.string(), std::ios_base::in | std::ios_base::binary);
    if (file.is_open())
    {
        std::filebuf* pbuf = file.rdbuf();
        std::size_t size = pbuf->pubseekoff(0, file.end, file.in);
        content.resize(size);
        pbuf->sgetn(content.data(), size);
        file.close();
        return true;
    }

    return false;
}

bool writeFile(const boost::filesystem::path& path, const std::string& content)
{
    if (fileExist(path) && !makeFileWritable(path))
    {
        return false;
    }

    std::fstream file;
    file.open(path.string(), std::ios_base::out | std::ios_base::trunc);
    if (file.is_open())
    {
        file << content;
        file.close();
        return true;
    }
    return false;
}
bool writeFile(const boost::filesystem::path& path, const std::vector<char>& content)
{
    if (fileExist(path) && !makeFileWritable(path))
    {
        return false;
    }

    std::fstream file;
    file.open(path.string(), std::ios_base::out | std::ios_base::trunc);
    if (file.is_open())
    {
        file.write(content.data(), content.size());
        file.close();
        return true;
    }
    return false;
}


}}