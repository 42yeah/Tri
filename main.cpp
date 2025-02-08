#include "TriApp.hpp"
#include "TriLog.hpp"

#include <iostream>
#include <memory>

int main()
{
    std::unique_ptr<TriApp> triApp = std::make_unique<TriApp>("Tri", 800, 600);

    triApp->Init();
    triApp->Loop();
    triApp->Finalize();
    
    return 0;
}
