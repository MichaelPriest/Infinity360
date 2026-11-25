#include "DiscImage.h"
#include "GameScanner.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>

namespace {
constexpr unsigned Sector = 2048;
constexpr uint64_t MaxImageBytes = 16ULL << 30;
constexpr char Magic[] = "MICROSOFT*XBOX*MEDIA";
uint64_t aligned(uint64_t n, unsigned unit) { return (n + unit - 1) / unit * unit; }
void le(unsigned char* p, uint32_t v, unsigned n) {
    for (unsigned i = 0; i < n; ++i) { p[i] = static_cast<unsigned char>(v); v >>= 8; }
}
// GDF sorts single-byte names case-insensitively, including Latin-1 letters.
std::string sortKey(std::string name) {
    for (char& c : name) {
        unsigned char b = static_cast<unsigned char>(c);
        if ((b >= 'a' && b <= 'z') || (b >= 0xe0 && b <= 0xfe && b != 0xf7)) c = char(b - 32);
    }
    return name;
}
bool fail(std::string& error, const char* message) { error = message; return false; }
}

bool DiscImage::LayoutRecords(Node& dir, const std::vector<size_t>& children, size_t first,
                              size_t end, uint32_t& cursor, uint16_t& link) {
    link = 0;
    if (first == end) return true;
    const size_t mid = first + (end - first) / 2;
    const size_t child = children[mid];
    const unsigned length = static_cast<unsigned>(aligned(14 + nodes_[child].name.size(), 4));
    if (cursor % Sector + length > Sector) cursor = static_cast<uint32_t>(aligned(cursor, Sector));
    if (cursor / 4 > 0xffff) return false;
    link = static_cast<uint16_t>(cursor / 4);
    const size_t record = dir.records.size();
    dir.records.push_back({child, cursor, 0, 0});
    cursor += length;
    uint16_t left, right;
    if (!LayoutRecords(dir, children, first, mid, cursor, left) ||
        !LayoutRecords(dir, children, mid + 1, end, cursor, right)) return false;
    dir.records[record].left = left;
    dir.records[record].right = right;
    return true;
}

bool DiscImage::ReadDirectory(size_t index, const FileWorkspace& ws, const Poll& poll,
                              unsigned depth, std::string& error) {
    if (!poll()) return fail(error, "Conversao cancelada.");
    std::vector<FolderEntry> entries;
    const std::string parent = nodes_[index].path;
    if (depth > 32 || !ws.CanBrowse(parent) || !ReadFolder(parent, entries))
        return fail(error, "Nao foi possivel ler uma pasta do DVD.");
    if (entries.size() > 16385 - nodes_.size()) return fail(error, "DVD excede 16384 entradas.");
    // Fold once per name. Sorting must be identical for files and directories.
    std::vector<std::pair<std::string, size_t>> sorted;
    for (const auto& e : entries) {
        if (!poll()) return fail(error, "Conversao cancelada.");
        if (e.name.empty() || e.name.size() > 255 || e.name == "." || e.name == ".." ||
            e.name.find_first_of("/\\:") != std::string::npos) return fail(error, "Nome de arquivo invalido no DVD.");
        Node node{e.name, FileWorkspace::Join(parent, e.name), e.directory, 0, 0, {}};
        if (!e.directory) {
            if (!ws.CanReadFile(node.path)) return fail(error, "Arquivo do DVD indisponivel.");
            std::error_code ec;
            const uint64_t bytes = std::filesystem::file_size(node.path, ec);
            if (ec || bytes > UINT32_MAX) return fail(error, "Tamanho de arquivo incompativel com XDVDFS.");
            node.size = static_cast<uint32_t>(bytes);
        }
        sorted.emplace_back(sortKey(e.name), nodes_.size());
        nodes_.push_back(std::move(node));
    }
    std::sort(sorted.begin(), sorted.end());
    std::vector<size_t> children;
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i && sorted[i - 1].first == sorted[i].first) return fail(error, "Nomes duplicados no DVD.");
        children.push_back(sorted[i].second);
    }
    uint32_t cursor = 0;
    uint16_t root = 0;
    if (!LayoutRecords(nodes_[index], children, 0, children.size(), cursor, root))
        return fail(error, "Tabela de diretorio excede o formato XDVDFS.");
    nodes_[index].size = static_cast<uint32_t>(aligned(cursor, Sector));
    for (size_t child : children)
        if (nodes_[child].directory && !ReadDirectory(child, ws, poll, depth + 1, error)) return false;
    return true;
}

bool DiscImage::Plan(const FileWorkspace& ws, const std::string& source, const Poll& poll, std::string& error) {
    nodes_.clear(); bytes_ = 0;
    nodes_.push_back({"", source, true, 0, 0, {}});
    if (!ReadDirectory(0, ws, poll, 0, error)) return false;
    uint64_t offset = 34 * Sector;
    for (auto& node : nodes_) {
        node.sector = node.size ? static_cast<uint32_t>(offset / Sector) : 0;
        offset += aligned(node.size, Sector);
        if (offset > MaxImageBytes) return fail(error, "DVD excede 16 GiB.");
    }
    bytes_ = aligned(offset, 4096);
    return true;
}

bool DiscImage::SameListing(const DiscImage& other) const {
    if (nodes_.size() != other.nodes_.size() || bytes_ != other.bytes_) return false;
    for (size_t i = 0; i < nodes_.size(); ++i) {
        const auto& a = nodes_[i]; const auto& b = other.nodes_[i];
        if (a.path != b.path || a.directory != b.directory || a.size != b.size) return false;
    }
    return true;
}

bool DiscImage::Stream(const FileWorkspace& ws, const Sink& sink, std::string& error) const {
    if (!bytes_ || nodes_.empty()) return fail(error, "Volume DVD vazio.");
    std::vector<unsigned char> buffer(65536, 0);
    if (!sink(buffer.data(), 32 * Sector)) return false;
    std::array<unsigned char, 2 * Sector> descriptor{};
    std::memcpy(descriptor.data(), Magic, 20);
    std::memcpy(descriptor.data() + Sector - 20, Magic, 20);
    le(descriptor.data() + 20, nodes_[0].sector, 4);
    le(descriptor.data() + 24, nodes_[0].size, 4);
    if (!sink(descriptor.data(), descriptor.size())) return false;
    uint64_t offset = 34 * Sector;
    const std::array<unsigned char, 4096> zeros{};
    for (const auto& node : nodes_) {
        if (node.directory) {
            if (!node.size) continue;
            std::vector<unsigned char> table(node.size, 0xff);
            for (const auto& record : node.records) {
                auto* out = table.data() + record.offset;
                const Node& child = nodes_[record.node];
                le(out, record.left, 2); le(out + 2, record.right, 2);
                le(out + 4, child.sector, 4); le(out + 8, child.size, 4);
                out[12] = child.directory ? 0x10 : 0x20;
                out[13] = static_cast<unsigned char>(child.name.size());
                std::memcpy(out + 14, child.name.data(), child.name.size());
            }
            if (!sink(table.data(), table.size())) return false;
        } else {
            if (!ws.CanReadFile(node.path)) return fail(error, "DVD removido ou arquivo indisponivel.");
            std::ifstream file(node.path, std::ios::binary);
            if (!file) return fail(error, "Falha ao abrir arquivo do DVD.");
            for (uint32_t left = node.size; left;) {
                const size_t n = std::min<size_t>(left, buffer.size());
                if (!file.read(reinterpret_cast<char*>(buffer.data()), n)) return fail(error, "Falha na leitura do DVD.");
                if (!sink(buffer.data(), n)) return false;
                left -= static_cast<uint32_t>(n);
            }
            if (file.peek() != std::char_traits<char>::eof() || file.bad()) return fail(error, "Arquivo do DVD mudou.");
            const size_t padding = static_cast<size_t>(aligned(node.size, Sector) - node.size);
            if (padding && !sink(zeros.data(), padding)) return false;
        }
        offset += aligned(node.size, Sector);
    }
    return bytes_ == offset || sink(zeros.data(), static_cast<size_t>(bytes_ - offset));
}
