#include "Fileutil.h"
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/system/detail/error_code.hpp>

#include <fstream>
#include <ios>
#include <sstream>
#include <streambuf>


namespace Util {

std::_Iosb<int>::_Openmode convertOpenMode(File::eFileOpenMode mode)
{
    switch (mode) {
    case File::eFileOpenMode::kBinary:
        return std::ios_base::binary;
    case File::eFileOpenMode::kText:
    default:
        return static_cast<std::_Iosb<int>::_Openmode>(0x00);
    }
}

static boost::filesystem::path g_exePath = "";
static boost::filesystem::path g_resourcePath = "";

bool File::setExePath(const boost::filesystem::path& path)
{
    g_exePath = path;
    return true;
}
boost::filesystem::path File::getExePath()
{
    return g_exePath;
}

bool File::setResourcePath(const boost::filesystem::path& path)
{
    g_resourcePath = path;
    return true;
}
boost::filesystem::path File::getResourcePath()
{
    return g_resourcePath;
}

bool File::fileExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat)
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

bool File::dirExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat)
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

bool File::fileOrDirExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat)
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


bool File::canRead(const boost::filesystem::path& path)
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

bool File::canWrite(const boost::filesystem::path& path)
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

bool File::makeFileReadable(const boost::filesystem::path& path)
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

bool File::makeFileWritable(const boost::filesystem::path& path)
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

bool File::readFile(const boost::filesystem::path& path, std::string &content, eFileOpenMode mode)
{
    if (!fileExist(path))
    {
        return false;
    }

    if (!canRead(path) && !makeFileReadable(path))
    {
        return false;
    }

    auto openMode = std::ios_base::in | convertOpenMode(mode);

    std::fstream file;
    file.open(path.string(), openMode);
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

bool File::readFile(const boost::filesystem::path& path, std::vector<char> &content, eFileOpenMode mode)
{
    if (!fileExist(path))
    {
        return false;
    }

    if (!canRead(path) && !makeFileReadable(path))
    {
        return false;
    }

    auto openMode = std::ios_base::in | convertOpenMode(mode);

    std::fstream file;
    file.open(path.string(), openMode);
    if (file.is_open())
    {
        std::filebuf* pbuf = file.rdbuf();
        std::size_t size = pbuf->pubseekoff(0, file.end, openMode);
        content.resize(size);
        std::size_t readedNum = 0;
        while (readedNum != size)
        {
            pbuf->pubseekpos(readedNum);
            readedNum += pbuf->sgetn(&content.data()[readedNum], size);
        }
        file.close();
        return true;
    }

    return false;
}

bool File::writeFile(const boost::filesystem::path& path, const std::string& content, eFileOpenMode mode, bool trunc)
{
    return File::writeFile(path, (const unsigned char*)content.data(), content.size(), mode, trunc);
}

bool File::writeFile(const boost::filesystem::path& path, const std::vector<char>& content, eFileOpenMode mode, bool trunc)
{
    return File::writeFile(path, (const unsigned char*)content.data(), content.size(), mode, trunc);
}

bool File::writeFile(const boost::filesystem::path& path, const unsigned char* filecontent, std::size_t filesize, eFileOpenMode mode, bool trunc)
{
    if (fileExist(path) && !makeFileWritable(path))
    {
        return false;
    }

    auto openMode = std::ios_base::out | convertOpenMode(mode);
    if (trunc)
    {
        openMode |= std::ios_base::trunc;
    }

    std::fstream file;
    file.open(path.string(), openMode);
    if (file.is_open())
    {
        file.write((char*)filecontent, filesize);
        file.close();
        return true;
    }
    return false;
}

}