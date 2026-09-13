#define SDL_MAIN_HANDLED 1
#define NOMINMAX
#include "Application.h"
#include <Logger/Logger.h>
#include <SDL.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {
    std::string PathText(const std::filesystem::path& path)
    {
        const auto utf8 = path.u8string();
        return std::string(utf8.begin(), utf8.end());
    }

    std::filesystem::path ExecutablePath()
    {
#ifdef _WIN32
        std::vector<wchar_t> buffer(512);
        while (buffer.size() <= 32768) {
            const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (length == 0)
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Cannot locate EDITOR.exe");
            if (length < buffer.size())
                return std::filesystem::path(std::wstring(buffer.data(), length));
            buffer.resize(buffer.size() * 2);
        }
        throw std::runtime_error("The executable path exceeds the supported Windows path length.");
#else
        char* base = SDL_GetBasePath();
        if (!base)
            throw std::runtime_error(std::string("Cannot locate the executable: ") + SDL_GetError());
        const auto directory = std::filesystem::u8path(base);
        SDL_free(base);
        return directory / "EDITOR";
#endif
    }

    void SetEditorWorkingDirectory(const std::filesystem::path& executable)
    {
        for (auto directory = executable.parent_path(); !directory.empty();) {
            const auto editor = directory / "EDITOR";
            if (std::filesystem::is_regular_file(directory / "The 2D Engine.sln") &&
                std::filesystem::is_directory(editor / "assets")) {
                std::filesystem::current_path(editor);
                return;
            }
            const auto parent = directory.parent_path();
            if (parent == directory)
                break;
            directory = parent;
        }
        throw std::runtime_error(
            "Cannot find the repository's EDITOR/assets directory.\n"
            "Keep EDITOR.exe inside the complete Engine-Demo checkout.\n"
            "Executable: " + PathText(executable));
    }

    void ReportStartupFailure(const std::string& message)
    {
        std::cerr << "Engine-Demo startup failed:\n" << message << std::endl;
#ifdef _WIN32
        const int count = MultiByteToWideChar(CP_UTF8, 0, message.data(), static_cast<int>(message.size()), nullptr, 0);
        std::wstring text(count, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, message.data(), static_cast<int>(message.size()), text.data(), count);
        MessageBoxW(nullptr, text.c_str(), L"Engine-Demo startup failed", MB_OK | MB_ICONERROR);
#else
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine-Demo startup failed", message.c_str(), nullptr);
#endif
    }

    std::string InitializationErrors()
    {
        std::string message;
        int count = 0;
        for (const auto& entry : ENGINE_GET_LOGS()) {
            if (entry.type == ENGINE_LOGGER::LogEntry::LogType::ERR) {
                message += entry.log + "\n";
                if (++count == 4)
                    break;
            }
        }
        if (message.empty())
            message = "Application initialization failed.\n";
        return message + "\nResource working directory: " + PathText(std::filesystem::current_path());
    }
}

int main()
{
#ifdef _WIN32
#ifndef NDEBUG
    ShowWindow(GetConsoleWindow(), SW_SHOW);
#else
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
#endif

    try {
        SetEditorWorkingDirectory(ExecutablePath());
        const int result = ENGINE_EDITOR::Application::GetInstance().Run();
        if (result != EXIT_SUCCESS)
            ReportStartupFailure(InitializationErrors());
        return result;
    }
    catch (const std::exception& error) {
        ReportStartupFailure(error.what());
        return EXIT_FAILURE;
    }
}
