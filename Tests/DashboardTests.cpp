#include "Dashboard.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

int main() {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "aurorax_test_games";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "Halo 3");
    std::ofstream(root / "Halo 3" / "default.xex") << "fixture";

    GameScanner scanner({root.string()});
    const std::vector<Game> games = scanner.Scan();
    assert(games.size() == 1);
    assert(games.front().name == "Halo 3");

    Dashboard dashboard(scanner);
    dashboard.Initialize();
    assert(dashboard.SelectedSection() == "Jogos");
    Input input;
    input.SetButton(Input::Button::Right);
    dashboard.Update(input);
    assert(dashboard.SelectedSection() == "Aplicativos");
    std::ostringstream rendered;
    dashboard.Render(rendered);
    assert(rendered.str().find("AURORAX") != std::string::npos);

    std::filesystem::remove_all(root);
}
