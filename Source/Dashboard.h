#pragma once

#include "Input.h"


class Dashboard
{

public:

    void Initialize();

    void Update(Input input);

    void Render();


private:

    int currentMenu;

};
