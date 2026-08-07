#include "Dashboard.h"
#include "Input.h"

#include <iostream>
#include <string>

int main() {
    Dashboard dashboard;
    Input input;
    dashboard.Initialize();

    while (dashboard.Running()) {
        dashboard.Render(std::cout);
        std::cout << "> " << std::flush;
        std::string command;
        if (!(std::cin >> command)) break;
        input.SetButton(Input::FromText(command));
        dashboard.Update(input);
        input.Update();
    }
    return 0;
}
