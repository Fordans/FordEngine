#include "FDE/pch.hpp"
#include "FDE/Asset/AssetPack.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>

namespace FDE
{

namespace
{
constexpr char MAGIC[4] = {'F', 'D', 'E', '\x01'};
constexpr uint32_t FORMAT_VERSION = 1;

#pragma pack(push, 1)
struct FileHeader
{
    char magic[4];
    uint32_t version;
    uint32_t count;
};

struct EntryHeader
{
    char guid[36];
    uint8_t type;
    uint8_t pad[3];
    uint32_t pathLen;
    uint32_t blobLen;
};
#pragma pack(pop)

} // namespace

bool AssetPackWriter::WriteFile(const std::string& absolutePath, std::vector<AssetPackEntry> entries,
                                std::string& outError)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(fs::path(absolutePath).parent_path(), ec);

    FileHeader fh{};
    std::memcpy(fh.magic, MAGIC, 4);
    fh.version = FORMAT_VERSION;
    fh.count = static_cast<uint32_t>(entries.size());

    std::vector<EntryHeader> ehs;
    ehs.reserve(entries.size());
    std::vector<uint8_t> pathBlob;
    std::vector<uint8_t> dataBlob;

    for (AssetPackEntry& e : entries)
    {
        std::string g = e.guid.str();
        if (g.size() != 36)
        {
            outError = "Invalid GUID in pack entry";
            return false;
        }
        EntryHeader eh{};
        std::memcpy(eh.guid, g.data(), 36);
        eh.type = static_cast<uint8_t>(e.type);
        eh.pathLen = static_cast<uint32_t>(e.logicalPath.size());
        eh.blobLen = static_cast<uint32_t>(e.uncompressedBytes.size());
        ehs.push_back(eh);
        for (char c : e.logicalPath)
            pathBlob.push_back(static_cast<uint8_t>(c));
        dataBlob.insert(dataBlob.end(), e.uncompressedBytes.begin(), e.uncompressedBytes.end());
    }

    std::ofstream out(absolutePath, std::ios::binary);
    if (!out)
    {
        outError = "Failed to open fdepack for write";
        return false;
    }

    out.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
    out.write(reinterpret_cast<const char*>(ehs.data()),
              static_cast<std::streamsize>(ehs.size() * sizeof(EntryHeader)));
    out.write(reinterpret_cast<const char*>(pathBlob.data()),
              static_cast<std::streamsize>(pathBlob.size()));
    out.write(reinterpret_cast<const char*>(dataBlob.data()),
              static_cast<std::streamsize>(dataBlob.size()));
    out.close();
    return true;
}

bool AssetPackReader::OpenFile(const std::string& absolutePath, std::string& outError)
{
    Close();
    std::ifstream file(absolutePath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        outError = "Failed to open fdepack";
        return false;
    }
    std::streamsize sz = file.tellg();
    file.seekg(0);
    m_fileBytes.resize(static_cast<size_t>(sz));
    if (sz > 0)
        file.read(reinterpret_cast<char*>(m_fileBytes.data()), sz);
    file.close();

    if (m_fileBytes.size() < sizeof(FileHeader))
    {
        outError = "fdepack too small";
        return false;
    }

    FileHeader fh;
    std::memcpy(&fh, m_fileBytes.data(), sizeof(fh));
    if (std::memcmp(fh.magic, MAGIC, 4) != 0)
    {
        outError = "Bad fdepack magic";
        return false;
    }

    size_t off = sizeof(FileHeader);
    if (off + fh.count * sizeof(EntryHeader) > m_fileBytes.size())
    {
        outError = "Corrupt fdepack (TOC)";
        return false;
    }

    m_index.clear();
    m_toc.clear();

    std::vector<EntryHeader> ehs(fh.count);
    std::memcpy(ehs.data(), m_fileBytes.data() + off, fh.count * sizeof(EntryHeader));
    off += fh.count * sizeof(EntryHeader);

    for (uint32_t i = 0; i < fh.count; ++i)
    {
        const EntryHeader& eh = ehs[i];
        AssetPackEntry e;
        e.guid = AssetId(std::string(eh.guid, 36));
        e.type = static_cast<AssetType>(eh.type);
        if (eh.pathLen > 0)
        {
            if (off + eh.pathLen > m_fileBytes.size())
            {
                outError = "Corrupt fdepack (path)";
                Close();
                return false;
            }
            e.logicalPath.assign(reinterpret_cast<const char*>(m_fileBytes.data() + off), eh.pathLen);
            off += eh.pathLen;
        }
        m_index.push_back(std::move(e));
    }

    for (uint32_t i = 0; i < fh.count; ++i)
    {
        const EntryHeader& eh = ehs[i];
        TocRow tr;
        tr.offset = off;
        tr.size = eh.blobLen;
        if (off + eh.blobLen > m_fileBytes.size())
        {
            outError = "Corrupt fdepack (blob)";
            Close();
            return false;
        }
        off += eh.blobLen;
        m_toc.push_back(tr);
    }

    m_path = absolutePath;
    return true;
}

void AssetPackReader::Close()
{
    m_fileBytes.clear();
    m_index.clear();
    m_toc.clear();
    m_path.clear();
}

bool AssetPackReader::ReadBlob(size_t entryIndex, std::vector<uint8_t>& outBytes) const
{
    if (entryIndex >= m_toc.size() || entryIndex >= m_index.size())
        return false;
    size_t off = m_toc[entryIndex].offset;
    size_t sz = m_toc[entryIndex].size;
    if (off + sz > m_fileBytes.size())
        return false;
    outBytes.assign(m_fileBytes.data() + off, m_fileBytes.data() + off + sz);
    return true;
}

} // namespace FDE
