#include <QApplication>
#include <QWidget>
#include <cstdio>
#include <QFileDialog>
#include <filesystem>
#include <SDL2/SDL.h>

#include "emulator_window.hpp"
#include "settings.hpp"
#include "error.hpp"

using namespace SuperShuckie64;

#ifdef _WIN32
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin)
#endif

static int runapp(int argc, char ** argv) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("SnowyMouse");
    QCoreApplication::setApplicationName("SuperShuckie");

    std::error_code ec;
    auto path = get_applocal_path();
    std::filesystem::create_directories(path, ec);
    if(ec) {
        DISPLAY_ERROR_DIALOG("Can't initialize application settings!", "Can't create directories for %s\n\nMake sure you have permission!\n\nThe error was: %s", path.string().c_str(), ec.message().c_str());
        return 1;
    }

    EmulatorWindow window(argc >= 2 ? std::optional(std::filesystem::path(argv[1])) : std::nullopt);

    if(!window.is_valid()) {
        return 1;
    }

    window.show();

    return app.exec();
}

int main(int argc, char **argv) {
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
    int result = runapp(argc, argv);
    SDL_Quit();
    return result;
}

extern "C" void getline() {
    std::printf("What is getline()???\n");
    std::terminate();
}
