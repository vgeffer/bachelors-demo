#include "App/AppManifest.hpp"
#include <iostream>
#include <App/Application.hpp>

using App = pgr::App::Application;

int main(void) {
    pgr::App::AppManifest manifest = pgr::App::AppManifest()
        .SetVersion(0, 0, 1, 0)
        .SetName("Bachelors Demo")
        .SetAppRoot("");

    manifest.Flags.UseValidations = true;

    App::Status status = App::Status::eOk;

    try {
        App app = App(manifest);

        do {
            status = app.Start();
        } while (status == App::Status::eAppReload); /* Reload app */
    }
    
    /* Will only catch errors during initialization, others will return Status::Error*/
    catch (std::runtime_error e) {
        std::cerr << "\x1b[91m" << e.what() << "\x1b[0m" << std::endl;
        return -1;
    }
    
    return (int)status;
}


