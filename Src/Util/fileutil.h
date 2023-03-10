#pragma once

#include <cstddef>
#include <ios>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace Util { namespace File {

enum class eFileOpenMode
{
    kText, kBinary
};
bool setExePath(const boost::filesystem::path& path);
boost::filesystem::path getExePath();

bool setResourcePath(const boost::filesystem::path& path);
boost::filesystem::path getResourcePath();

bool fileExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat = nullptr);
bool dirExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat = nullptr);
bool fileOrDirExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat = nullptr);

bool canRead(const boost::filesystem::path& path);
bool canWrite(const boost::filesystem::path& path);

bool makeFileReadable(const boost::filesystem::path& path);
bool makeFileWritable(const boost::filesystem::path& path);

bool readFile(const boost::filesystem::path& path, std::string& content, eFileOpenMode mode = eFileOpenMode::kText);
bool readFile(const boost::filesystem::path& path, std::vector<char>& content, eFileOpenMode mode = eFileOpenMode::kBinary);

bool writeFile(const boost::filesystem::path& path, const std::string& content, eFileOpenMode mode = eFileOpenMode::kText, bool trunc = true);
bool writeFile(const boost::filesystem::path& path, const std::vector<char>& content, eFileOpenMode mode = eFileOpenMode::kBinary, bool trunc = true);
bool writeFile(const boost::filesystem::path& path, const unsigned char* file, std::size_t filesize, eFileOpenMode mode = eFileOpenMode::kBinary, bool trunc = true);

std::string getLowerExtension(const boost::filesystem::path& path);
}}