#include "Dashboard.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <ostream>

Dashboard::Dashboard(GameScanner scanner)
    : scanner_(std::move(scanner)), sections_({"Jogos", "Aplicativos", "Emuladores", "Arquivos", "Sistema", "Configuracoes"}),
      view_(View::Home), selectedSection_(0), selectedGame_(0), running_(true), showHidden_(false) {}

void Dashboard::Initialize() {
    games_ = scanner_.Scan();
    notification_ = std::to_string(games_.size()) + " jogo(s) encontrado(s)";
}

void Dashboard::OpenSelected() {
    if (selectedSection_ == 0) view_ = View::Library;
    else if (selectedSection_ == 3) view_ = View::FileManager;
    else if (selectedSection_ == 4) view_ = View::System;
    else if (selectedSection_ == 5) view_ = View::Settings;
    else notification_ = sections_[selectedSection_] + ": nenhum item instalado";
}

void Dashboard::Update(const Input& input) {
    notification_.clear();
    if (input.Back()) { running_ = false; return; }
    if (input.B()) { view_ = View::Home; return; }
    if (view_ == View::Home) {
        if (input.Left() && selectedSection_ > 0) --selectedSection_;
        if (input.Right() && selectedSection_ + 1 < sections_.size()) ++selectedSection_;
        if (input.A()) OpenSelected();
        if (input.Y()) { games_ = scanner_.Scan(); notification_ = "Biblioteca atualizada"; }
    } else if (view_ == View::Library || view_ == View::Details) {
        if (input.Up() && selectedGame_ > 0) --selectedGame_;
        if (input.Down() && selectedGame_ + 1 < games_.size()) ++selectedGame_;
        if (input.X() && !games_.empty()) { games_[selectedGame_].favorite = !games_[selectedGame_].favorite; notification_ = "Favorito atualizado"; }
        if (input.A() && !games_.empty()) { view_ = View::Details; notification_ = "Pronto para iniciar: " + games_[selectedGame_].executable; }
        if (input.Y()) { games_ = scanner_.Scan(); selectedGame_ = 0; notification_ = "Biblioteca atualizada"; }
    } else if (view_ == View::Settings && input.A()) {
        showHidden_ = !showHidden_;
        notification_ = "Exibir ocultos: " + std::string(showHidden_ ? "sim" : "nao");
    }
}

void Dashboard::Render(std::ostream& out) const {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    out << "\n=== INFINITY 360 === " << std::put_time(std::localtime(&now), "%H:%M") << "\n";
    if (view_ == View::Home) {
        out << "Inicio  |  Biblioteca: " << games_.size() << " titulo(s)\n\n";
        for (std::size_t i = 0; i < sections_.size(); ++i)
            out << (i == selectedSection_ ? "[ " : "  ") << sections_[i] << (i == selectedSection_ ? " ]" : "  ") << (i + 1 == sections_.size() ? "\n" : "  ");
        out << "\nA Entrar  Y Atualizar  Direcional Navegar  BACK Sair\n";
    } else if (view_ == View::Library || view_ == View::Details) {
        out << "Biblioteca de jogos\n";
        if (games_.empty()) out << "  Nenhum default.xex encontrado em Games/ ou USB/Games/.\n";
        for (std::size_t i = 0; i < games_.size(); ++i)
            out << (i == selectedGame_ ? "> " : "  ") << (games_[i].favorite ? "* " : "  ") << games_[i].name << "  (" << games_[i].device << ")\n";
        if (view_ == View::Details && !games_.empty()) out << "Executavel: " << games_[selectedGame_].executable << "\n";
        out << "A Detalhes/Iniciar  X Favorito  Y Reescanear  B Inicio\n";
    } else if (view_ == View::System) {
        out << "Sistema\n  Rede: adapter XDK\n  Armazenamento: HDD/USB\n  Temperaturas: provider SMC pendente\n  Informacoes: CPU/GPU/RAM/Title ID\nB Inicio\n";
    } else if (view_ == View::FileManager) {
        out << "Gerenciador de arquivos\n  HDD1:  USB0:  USB1:  DVD:\n  Copiar, mover, renomear e excluir exigem confirmacao.\nB Inicio\n";
    } else {
        out << "Configuracoes\n  [A] Exibir arquivos ocultos: " << (showHidden_ ? "sim" : "nao") << "\n  Tema, idioma, rede, caminhos e controle parental\nB Inicio\n";
    }
    if (!notification_.empty()) out << "\n! " << notification_ << "\n";
}

bool Dashboard::Running() const { return running_; }
std::string Dashboard::SelectedSection() const { return sections_[selectedSection_]; }
