#include "Input.h"

#include <algorithm>
#include <cctype>

Input::Input() : button_(Button::None) {}

void Input::Update() { button_ = Button::None; }
void Input::SetButton(Button button) { button_ = button; }

Input::Button Input::FromText(const std::string& text) {
    std::string key = text;
    std::transform(key.begin(), key.end(), key.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (key == "a") return Button::A;
    if (key == "b") return Button::B;
    if (key == "x") return Button::X;
    if (key == "y") return Button::Y;
    if (key == "up" || key == "w") return Button::Up;
    if (key == "down" || key == "s") return Button::Down;
    if (key == "left") return Button::Left;
    if (key == "right") return Button::Right;
    if (key == "start") return Button::Start;
    if (key == "back" || key == "q") return Button::Back;
    return Button::None;
}

bool Input::A() const { return button_ == Button::A; }
bool Input::B() const { return button_ == Button::B; }
bool Input::X() const { return button_ == Button::X; }
bool Input::Y() const { return button_ == Button::Y; }
bool Input::Up() const { return button_ == Button::Up; }
bool Input::Down() const { return button_ == Button::Down; }
bool Input::Left() const { return button_ == Button::Left; }
bool Input::Right() const { return button_ == Button::Right; }
bool Input::Start() const { return button_ == Button::Start; }
bool Input::Back() const { return button_ == Button::Back; }
