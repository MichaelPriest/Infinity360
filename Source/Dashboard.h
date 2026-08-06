#pragma once

#include "GameScanner.h"
#include "Input.h"

#include <iosfwd>
#include <string>
#include <vector>

class Dashboard {
public:
    explicit Dashboard(GameScanner scanner = GameScanner());
    void Initialize();
    void Update(const Input& input);
    void Render(std::ostream& output) const;
    bool Running() const;
    std::string SelectedSection() const;

private:
    enum class View { Home, Library, Details, Settings, System, FileManager };
    void OpenSelected();

    GameScanner scanner_;
    std::vector<Game> games_;
    std::vector<std::string> sections_;
    View view_;
    std::size_t selectedSection_;
    std::size_t selectedGame_;
    bool running_;
    bool showHidden_;
    std::string notification_;
};
