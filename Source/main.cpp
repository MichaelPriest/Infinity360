#include "Dashboard.h"
#include "Input.h"


int main()
{

    Dashboard dashboard;
    Input input;


    dashboard.Initialize();


    while(true)
    {

        input.Update();

        dashboard.Update(input);

        dashboard.Render();

    }


    return 0;
}
