#pragma once

#include <cstddef>
#include <string>

// Portable controller state.  The XDK adapter only needs to translate XInput
// button edges into this small interface.
class Input {
public:
    enum class Button { None, A, B, X, Y, Up, Down, Left, Right, Start, Back };

    Input();
    void Update();
    void SetButton(Button button);
    static Button FromText(const std::string& text);

    bool A() const;
    bool B() const;
    bool X() const;
    bool Y() const;
    bool Up() const;
    bool Down() const;
    bool Left() const;
    bool Right() const;
    bool Start() const;
    bool Back() const;

private:
    Button button_;
};
