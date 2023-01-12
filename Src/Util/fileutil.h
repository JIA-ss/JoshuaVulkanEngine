#pragma once

#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace util { namespace file {

bool fileExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat = nullptr);
bool dirExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat = nullptr);
bool fileOrDirExist(const boost::filesystem::path& path, boost::filesystem::file_status* stat = nullptr);

bool canRead(const boost::filesystem::path& path);
bool canWrite(const boost::filesystem::path& path);

bool makeFileReadable(const boost::filesystem::path& path);
bool makeFileWritable(const boost::filesystem::path& path);

bool readFile(const boost::filesystem::path& path, std::string& content);
bool readFile(const boost::filesystem::path& path, std::vector<char>& content);

bool writeFile(const boost::filesystem::path& path, const std::string& content);
bool writeFile(const boost::filesystem::path& path, const std::vector<char>& content);

}}