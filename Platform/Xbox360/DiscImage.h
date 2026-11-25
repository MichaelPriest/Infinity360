#pragma once
#include "FileWorkspace.h"
#include <cstdint>
#include <functional>

// A compact XDVDFS volume made from the directory tree exposed by the DVD driver.
// File data is streamed directly to the caller. Only directory metadata is retained.
class DiscImage {
public:
    using Poll = std::function<bool()>;
    using Sink = std::function<bool(const unsigned char*, size_t)>;
    bool Plan(const FileWorkspace& workspace, const std::string& source, const Poll& poll,
              std::string& error);
    bool Stream(const FileWorkspace& workspace, const Sink& sink, std::string& error) const;
    bool SameListing(const DiscImage& other) const;
    uint64_t Bytes() const { return bytes_; }
private:
    struct Record { size_t node; uint32_t offset; uint16_t left, right; };
    struct Node {
        std::string name, path;
        bool directory;
        uint32_t size = 0, sector = 0;
        std::vector<Record> records;
    };
    std::vector<Node> nodes_;
    uint64_t bytes_ = 0;
    bool ReadDirectory(size_t index, const FileWorkspace& workspace, const Poll& poll,
                       unsigned depth, std::string& error);
    bool LayoutRecords(Node& dir, const std::vector<size_t>& children, size_t first,
                       size_t end, uint32_t& cursor, uint16_t& link);
};
