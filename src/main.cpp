#include <QApplication>
#include <QWidget>
#include <cstdio>
#include <QFileDialog>
#include <filesystem>
#include <QProcess>
#include <thread>
#include <fstream>
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

// TODO: make a better library for this
#ifdef _WIN32

#include <windows.h>
extern "C" void RR_PrintBacktrace();

static void exception_handler_stacktrace() {
    std::fprintf(stderr, "A fatal error occurred. Here's the stack trace:\n");
    std::fflush(stderr);
    RR_PrintBacktrace();
    std::fprintf(stderr, "SHUCKIE fainted!\n");
    std::fflush(stderr);
}

static void exception_handler_signal(int c) {
    switch(c) {
        case SIGSEGV: {
            std::fprintf(stderr, "Segmentation fault detected!\n");
            break;
        }
        case SIGABRT: {
            std::fprintf(stderr, "Abort signal received! (likely an unhandled exception somewhere)\n");
            break;
        }
    }
    exception_handler_stacktrace();
}

#endif



static int runapp(int argc, char ** argv) {
    // Create some logging stuff (TODO: CLEAN THIS UP!)
    bool is_real_process = false;
    if(std::strcmp(argv[argc - 1], "--real-process") == 0) {
        argc--;
        is_real_process = true;
    }

    #ifndef _WIN32
    is_real_process = true;
    #endif

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

    if(is_real_process) {
        EmulatorWindow window(argc >= 2 ? std::optional(std::filesystem::path(argv[1])) : std::nullopt);

        if(!window.is_valid()) {
            return 1;
        }

        #ifdef _WIN32
        signal(SIGABRT, exception_handler_signal);
        signal(SIGSEGV, exception_handler_signal);
        #endif

        SDL_Init(SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);

        window.show();
        auto result = app.exec();
        SDL_Quit();
        return result;
    }
    else {
        auto dir = std::filesystem::path(QDir::currentPath().toStdString());
        auto error = dir / "SuperShuckie64-logs.txt";

        QProcess process;
        process.setProgram(argv[0]);

        QStringList qsl;
        for(int i = 1; i < argc; i++) {
            qsl << argv[i];
        }
        qsl << "--real-process";

        {
            std::ofstream output;
            output.open(error, std::ios_base::app);
            output << std::endl;
            output << std::endl;
            output << "################################################################################" << std::endl;
            output << "Opened on: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toStdString() << std::endl;
        }

        process.setArguments(qsl);
        process.setStandardErrorFile(error.string().c_str(), QIODeviceBase::Append);

        auto environment = process.processEnvironment();
        environment.insert("RUST_BACKTRACE", "full");
        process.setProcessEnvironment(environment);

        process.start();

        while(process.state() != QProcess::NotRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            process.waitForFinished();
        }

        return process.exitCode();
    }
}

int main(int argc, char **argv) {
    int result = runapp(argc, argv);
    return result;
}

extern "C" void getline() {
    std::printf("What is getline()???\n");
    std::terminate();
}
