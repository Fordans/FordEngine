#pragma once

#include "FDE/Asset/AssetId.hpp"
#include "FDE/Asset/AssetType.hpp"
#include "FDE/Export.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace FDE
{

/// Single blob inside a .fdepack file.
struct FDE_API AssetPackEntry
{
    AssetId guid;
    AssetType type = AssetType::Unknown;
    std::string logicalPath;
    /// Payload (uncompressed for MVP; compSize in file equals size when uncompressed).
    std::vector<uint8_t> uncompressedBytes;
};

/// Writes binary `.fdepack` (see plan: header + TOC + blobs).
class FDE_API AssetPackWriter
{
  public:
    static bool WriteFile(const std::string& absolutePath, std::vector<AssetPackEntry> entries,
                          std::string& outError);
};

/// Read-only view of a pack (mmap-friendly sequential layout).
class FDE_API AssetPackReader
{
  public:
    bool OpenFile(const std::string& absolutePath, std::string& outError);
    void Close();

    const std::vector<AssetPackEntry>& GetIndex() const { return m_index; }

    /// Read blob bytes for entry i (copies from memory if file loaded fully).
    bool ReadBlob(size_t entryIndex, std::vector<uint8_t>& outBytes) const;

    bool IsOpen() const { return !m_fileBytes.empty() || !m_path.empty(); }

  private:
    struct TocRow
    {
        size_t offset = 0;
        size_t size = 0;
    };
    std::string m_path;
    std::vector<uint8_t> m_fileBytes;
    std::vector<AssetPackEntry> m_index;
    std::vector<TocRow> m_toc;
};

} // namespace FDE
