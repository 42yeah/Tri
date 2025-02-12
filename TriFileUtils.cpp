#include "TriFileUtils.hpp"
#include "TriLog.hpp"

#include <fstream>

std::optional<std::vector<char> > ReadBinaryFile(const std::string &path)
{
    std::ifstream reader(path, std::ios::binary);
    
    if (!reader.good())
    {
        return std::nullopt;
    }

    // Seek to end
    reader.seekg(0, reader.end);
    ssize_t fileSize = reader.tellg();
    reader.seekg(0, reader.beg);

    std::vector<char> ret(fileSize);
    reader.read(ret.data(), ret.size());

    if (reader.eof())
    {
        TriLogWarning() << "Reached EOF while trying to read " << path;
    }
    else if (reader.tellg() != fileSize)
    {
        TriLogWarning() << "Incomplete read for " << path;
    }

    return ret;
}
