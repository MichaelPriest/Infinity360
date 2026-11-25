#include "DiscInstaller.h"
#include "DiscImage.h"
#include "GameScanner.h"
#include "PackageImport.h"
#include "StorageInfo.h"
#include "XexLaunchCheck.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <fcntl.h>
#include <unistd.h>
#ifdef __XSDK__
#include <mbedtls/sha1.h>
#else
#include <openssl/sha.h>
#endif

namespace {
using Bytes = unsigned long long;
using Progress = std::function<bool(Bytes, Bytes)>;
constexpr size_t Block = 0x1000, BlocksPerGroup = 0xcc, GroupsPerPart = 0xcb;
constexpr size_t GroupBytes = Block * BlocksPerGroup, HeaderBytes = 0xb000;
using Table = std::array<unsigned char, Block>;
using Digest = std::array<unsigned char, 20>;
struct CloseFile { void operator()(FILE* f) const { std::fclose(f); } };
using File = std::unique_ptr<FILE, CloseFile>;
bool fail(std::string& error, const char* message) { error = message; return false; }
bool hash(const unsigned char* bytes, size_t n, unsigned char* out) {
#ifdef __XSDK__
    return mbedtls_sha1(bytes, n, out) == 0;
#else
    return SHA1(bytes, n, out) != nullptr;
#endif
}
void be(unsigned char* field, uint64_t n, unsigned width) {
    while (width) { field[--width] = static_cast<unsigned char>(n); n >>= 8; }
}
Bytes ceilDiv(Bytes n, size_t per) { return (n + per - 1) / per; }
Bytes diskBytes(Bytes payload) {
    const Bytes groups = ceilDiv(payload, GroupBytes);
    return payload + (groups + ceilDiv(groups, GroupsPerPart)) * Block;
}
bool writeAll(FILE* f, const void* data, size_t n) { return std::fwrite(data, 1, n, f) == n; }
bool finishFile(File& f, bool sync) {
    const bool flushed = std::fflush(f.get()) == 0;
    const bool synced = !sync || fsync(fileno(f.get())) == 0;
    const bool closed = std::fclose(f.release()) == 0;
    return flushed && synced && closed;
}

// Track only objects created by this installation. Incomplete stages are outside the catalog.
struct Stage {
    std::string root;
    std::vector<std::string> created;
    ~Stage() {
        if (root.empty()) return;
        std::error_code ec;
        for (auto it = created.rbegin(); it != created.rend(); ++it) std::filesystem::remove(*it, ec);
        std::filesystem::remove(root, ec);
    }
    bool Directory(const std::string& path) {
        std::error_code ec;
        if (!std::filesystem::create_directory(path, ec)) return false;
        created.push_back(path);
        return true;
    }
    File Create(const std::string& path) {
        int flags = O_RDWR | O_CREAT | O_EXCL;
#ifndef __XSDK__
        flags |= O_NOFOLLOW;
#endif
        int fd = open(path.c_str(), flags, 0600);
        if (fd < 0) return {};
        created.push_back(path);
        File file(fdopen(fd, "w+b"));
        if (!file) close(fd);
        return file;
    }
};

class SvodWriter {
public:
    SvodWriter(Stage& stage, const std::string& folder, Bytes total, const Progress& progress, std::string& error)
        : stage_(stage), folder_(folder), total_(total), progress_(progress), error_(error), group_(GroupBytes) {}
    bool Append(const unsigned char* bytes, size_t n) {
        if (n > total_ - done_) return fail(error_, "Volume excedeu o tamanho previsto.");
        while (n) {
            const size_t count = std::min(n, GroupBytes - used_);
            std::memcpy(group_.data() + used_, bytes, count);
            bytes += count; n -= count; used_ += count; done_ += count;
            if (used_ == GroupBytes && !FlushGroup()) return false;
            if (!Poll()) return false;
        }
        return true;
    }
    bool Finish(Digest& root) {
        if (done_ != total_ || used_ % Block) return fail(error_, "Volume XDVDFS incompleto.");
        if (used_ && !FlushGroup()) return false;
        if (file_ && !finishFile(file_, false)) return fail(error_, "Erro ao fechar fragmento GOD.");
        root.fill(0);
        for (size_t p = masters_.size(); p-- > 0;) {
            if (!Poll()) return false;
            Table& master = masters_[p];
            if (p + 1 < masters_.size()) std::copy(root.begin(), root.end(), master.begin() + GroupsPerPart * root.size());
            File file(std::fopen(Part(p).c_str(), "r+b"));
            if (!file || !writeAll(file.get(), master.data(), Block) || !finishFile(file, true) ||
                !hash(master.data(), Block, root.data())) return fail(error_, "Falha ao gravar tabela mestre GOD.");
        }
        return Verify(root);
    }
    size_t Parts() const { return masters_.size(); }
private:
    Stage& stage_;
    std::string folder_;
    Bytes total_, done_ = 0;
    const Progress& progress_;
    std::string& error_;
    std::vector<unsigned char> group_;
    size_t used_ = 0, groupIndex_ = 0;
    std::vector<Table> masters_;
    File file_;
    bool Poll() { return progress_(done_, total_ * 2) || fail(error_, "Conversao cancelada."); }
    std::string Part(size_t index) const {
        char name[16]; std::snprintf(name, sizeof name, "Data%04u", static_cast<unsigned>(index));
        return FileWorkspace::Join(folder_, name);
    }
    bool FlushGroup() {
        if (!file_) {
            const auto path = Part(masters_.size());
            file_ = stage_.Create(path);
            masters_.push_back(Table{});
            if (!file_ || !writeAll(file_.get(), masters_.back().data(), Block)) return fail(error_, "Falha ao criar fragmento GOD.");
            groupIndex_ = 0;
        }
        Table hashes{};
        for (size_t offset = 0; offset < used_; offset += Block)
            if (!hash(group_.data() + offset, Block, hashes.data() + offset / Block * 20)) return fail(error_, "Falha no SHA-1 do bloco.");
        if (!hash(hashes.data(), Block, masters_.back().data() + groupIndex_ * 20) ||
            !writeAll(file_.get(), hashes.data(), Block) || !writeAll(file_.get(), group_.data(), used_))
            return fail(error_, "Erro ao gravar dados GOD.");
        used_ = 0;
        if (++groupIndex_ == GroupsPerPart && !finishFile(file_, false)) return fail(error_, "Falha ao fechar fragmento GOD.");
        return true;
    }
    bool Verify(Digest expected) {
        Bytes left = total_;
        for (size_t p = 0; p < masters_.size(); ++p) {
            File f(std::fopen(Part(p).c_str(), "rb"));
            Table master, hashes;
            Digest digest;
            if (!f || std::fread(master.data(), 1, Block, f.get()) != Block ||
                !hash(master.data(), Block, digest.data()) || digest != expected) return fail(error_, "Cadeia de fragmentos GOD divergente.");
            for (size_t g = 0; g < GroupsPerPart && left; ++g) {
                const size_t bytes = static_cast<size_t>(std::min<Bytes>(GroupBytes, left));
                if (std::fread(hashes.data(), 1, Block, f.get()) != Block || !hash(hashes.data(), Block, digest.data()) ||
                    std::memcmp(master.data() + g * 20, digest.data(), 20) ||
                    std::fread(group_.data(), 1, bytes, f.get()) != bytes) return fail(error_, "Tabela ou fragmento GOD divergente.");
                for (size_t at = 0; at < bytes; at += Block) {
                    if (!hash(group_.data() + at, Block, digest.data()) ||
                        std::memcmp(hashes.data() + at / Block * 20, digest.data(), 20)) return fail(error_, "Releitura GOD divergente.");
                }
                left -= bytes; done_ += bytes;
                if (!Poll()) return false;
            }
            if (std::fgetc(f.get()) != EOF || std::ferror(f.get())) return fail(error_, "Tamanho de fragmento GOD divergente.");
            std::copy_n(master.data() + GroupsPerPart * 20, 20, expected.data());
        }
        return !left || fail(error_, "Fragmento GOD ausente.");
    }
};

bool headerFile(Stage& stage, const std::string& path, const unsigned char* execution, const Digest& root,
                Bytes size, size_t parts, const std::string& name) {
    std::vector<unsigned char> header(HeaderBytes, 0);
    std::memcpy(header.data(), "LIVE", 4);
    be(&header[0x340], 0xad0e, 4); be(&header[0x344], 0x7000, 4); be(&header[0x348], 2, 4);
    be(&header[0x34c], size, 8);
    std::copy_n(execution, 24, &header[0x354]);
    header[0x379] = 0x24;
    header[0x37a] = 5; header[0x37b] = 5; header[0x37c] = 0x11;
    std::copy(root.begin(), root.end(), &header[0x37d]);
    // SVOD block count and start use little-endian 24-bit fields; XContent sizes use big endian.
    const Bytes blocks = size / Block;
    for (unsigned i = 0; i < 3; ++i) header[0x392 + i] = static_cast<unsigned char>(blocks >> (8 * i));
    be(&header[0x39d], parts, 4); be(&header[0x3a1], diskBytes(size), 8); be(&header[0x3a9], 1, 4);
    for (size_t i = 0; i < name.size(); ++i) {
        header[0x411 + i * 2 + 1] = name[i]; header[0x1691 + i * 2 + 1] = name[i];
    }
    // The digest covers the padded metadata. Signing and license fields are left empty.
    if (!hash(&header[0x344], header.size() - 0x344, &header[0x32c])) return false;
    File f = stage.Create(path);
    if (!f || !writeAll(f.get(), header.data(), header.size()) || !finishFile(f, true)) return false;
    std::vector<unsigned char> check(HeaderBytes);
    File in(std::fopen(path.c_str(), "rb"));
    return in && std::fread(check.data(), 1, check.size(), in.get()) == check.size() &&
           std::fgetc(in.get()) == EOF && !std::ferror(in.get()) && check == header;
}
bool dvdEnabled(const std::string& root) {
    std::ifstream f(FileWorkspace::Join(root, "enable-dvd-install.txt"));
    std::string marker;
    return std::getline(f, marker) && marker == "ENABLE-DVD-INSTALL";
}
}

bool InstallDiscGod(const FileWorkspace& ws, const std::string& appRoot, const std::string& source,
                    const std::string& mount, const Progress& progress, std::string& installed,
                    std::string& error, const XboxExecutionInfo& expected) {
    installed.clear(); error.clear();
#ifdef __XSDK__
    if (!GameScanner::SameRoot(source, "dvd:/") ||
        (mount != "hdd:/" && mount != "usb0:/" && mount != "usb1:/" && mount != "usb2:/"))
        return fail(error, "Origem ou destino DVD invalido.");
#endif
    const auto personal = ws.WorkspaceFor(mount);
    if (!dvdEnabled(appRoot) || personal.empty()) return fail(error, "Habilite a escrita e a instalacao de DVD.");
    const auto xex = FileWorkspace::Join(source, "default.xex");
    ContentCatalog::ExecutionBytes execution, finalExecution;
    XboxExecutionInfo info{};
    auto identity = [&](ContentCatalog::ExecutionBytes& raw) {
        std::ifstream file(xex, std::ios::binary);
        return ws.CanReadFile(xex) && XexLaunchCheck::Header(file) &&
               ContentCatalog::ReadExecutionInfo(xex, info, &raw) && DiscIdentity::Same(expected, info);
    };
    if (!identity(execution)) return fail(error, "DVD alterado ou default.xex invalido.");
    DiscImage image;
    if (!image.Plan(ws, source, [&] { return progress(0, 0); }, error)) return false;
    const auto volume = StorageInfo::Read(mount);
    if (!volume.available || volume.writableBytes < diskBytes(image.Bytes()) + HeaderBytes + (16ULL << 20))
        return fail(error, "Espaco insuficiente no destino GOD.");
    if (!ws.CanWrite(personal) && !ws.Create(mount, personal.substr(personal.find_last_of("/\\") + 1)))
        return fail(error, "Area pessoal indisponivel.");
    const auto parent = FileWorkspace::Join(personal, "PackageLibrary");
    if (!ws.CanWrite(parent) && !ws.Create(personal, "PackageLibrary")) return fail(error, "Biblioteca GOD indisponivel.");
    if (PackageImport::Slots(personal).size() >= 256) return fail(error, "Biblioteca deste dispositivo cheia.");
    char slot[20], title[9], media[9];
    std::snprintf(slot, sizeof slot, "%08x%08x%02x", expected.titleId, expected.mediaId, expected.discNumber);
    std::snprintf(title, sizeof title, "%08X", expected.titleId);
    std::snprintf(media, sizeof media, "%08X", expected.mediaId);
    Stage stage;
    if (!ws.BeginDiscStage(parent, slot, stage.root)) return fail(error, "Este DVD ja existe no destino ou o destino esta indisponivel.");
    const auto titleDir = FileWorkspace::Join(stage.root, title);
    const auto typeDir = FileWorkspace::Join(titleDir, "00007000");
    const auto header = FileWorkspace::Join(typeDir, media), data = header + ".data";
    if (!stage.Directory(titleDir) || !stage.Directory(typeDir) || !stage.Directory(data)) return fail(error, "Falha ao preparar pasta GOD.");
    SvodWriter writer(stage, data, image.Bytes(), progress, error);
    Digest root;
    if (!image.Stream(ws, [&](const unsigned char* buf, size_t n) { return writer.Append(buf, n); }, error) || !writer.Finish(root)) return false;
    const std::string display = std::string("DVD ") + title + " - Disco " + std::to_string(expected.discNumber);
    if (!headerFile(stage, header, execution, root, image.Bytes(), writer.Parts(), display) ||
        ContentCatalog::CheckPackage(header).status != XboxPackageCheck::StructurePresent) return fail(error, "Cabecalho GOD divergente.");
    DiscImage current;
    if (!current.Plan(ws, source, [&] { return progress(2 * image.Bytes(), 2 * image.Bytes()); }, error)) return false;
    if (!image.SameListing(current) || !identity(finalExecution) || std::memcmp(execution, finalExecution, 24))
        return fail(error, "O DVD mudou durante a conversao.");
    if (!dvdEnabled(appRoot) || !ws.PublishDiscStage(parent, stage.root, slot)) return fail(error, "Falha ao publicar GOD no destino.");
    stage.root.clear();
    installed = FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(parent, slot), title), "00007000"), media);
    return true;
}
