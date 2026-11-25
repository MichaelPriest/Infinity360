#include "DiscInstaller.h"
#include "DiscSet.h"
#include "PackageImport.h"
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    namespace fs = std::filesystem;
    const std::string app = argv[1], mode = argv[2];
    const auto disc = fs::path(app) / "dvd", xex = disc / "default.xex";
    FileWorkspace ws(app);
    XboxExecutionInfo expected{};
    if (!ContentCatalog::ReadExecutionInfo(xex.string(), expected)) return 2;
    if (mode == "wrong") ++expected.discNumber;
    std::string installed, error;
    bool injected = false;
    auto progress = [&](unsigned long long done, unsigned long long total) {
        if (mode == "cancel-plan") return false;
        if (mode == "cancel-write" && done) return false;
        if (mode == "cancel-verify" && total && done > total / 2) return false;
        if (!injected && total && done == total / 2) {
            if (mode == "damage") {
                for (const auto& entry : fs::recursive_directory_iterator(fs::path(app) / "UserFiles")) {
                    if (entry.path().filename() != "Data0000") continue;
                    std::fstream file(entry.path(), std::ios::binary | std::ios::in | std::ios::out);
                    file.seekp(0x2000); file.put('!'); file.close(); injected = bool(file);
                }
            } else if (mode == "swap") {
                std::fstream file(xex, std::ios::binary | std::ios::in | std::ios::out);
                file.seekp(0x80 + 18); file.put(2); file.close(); injected = bool(file);
            }
        }
        return true;
    };
    if (!InstallDiscGod(ws, app, disc.string(), app, progress, installed, error, expected)) {
        std::cerr << error << '\n'; return 1;
    }
    auto packages = ContentCatalog(PackageImport::Roots(app)).Scan();
    std::vector<Game> games;
    for (const auto& p : packages) games.push_back({p.name, p.path, "Pacote GOD", false});
    const auto set = DiscSet::Find(games, expected);
    if (set.size() != packages.size()) return 3;
    PackageLaunch::Ticket ticket;
    const auto meta = ContentCatalog::DescribeTitle(installed, true);
    if (!PackageLaunch::Capture(installed, ticket) || !meta.known || meta.discNumber != expected.discNumber ||
        meta.discCount != expected.discCount || meta.baseVersion != expected.baseVersion) return 3;
    std::cout << installed << '\n';
}
