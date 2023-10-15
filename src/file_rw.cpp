#include <cstdio>

#include "error.hpp"
#include "file_rw.hpp"

std::optional<std::vector<std::byte>> SuperShuckie64::read_file(const std::filesystem::path &path) {
    auto path_cstr = path.string();
    auto *f = std::fopen(path_cstr.c_str(), "rb");

    if(!f) {
        DISPLAY_ERROR_DIALOG("Can't open file", "Can't open '%s' for reading!", path_cstr.c_str());
        return std::nullopt;
    }

    std::fseek(f, 0, SEEK_END);
    auto len = static_cast<std::size_t>(std::ftell(f));
    std::fseek(f, 0, SEEK_SET);

    std::vector<std::byte> final;
    final.resize(len);
    std::fread(final.data(), len, 1, f);
    std::fclose(f);

    return final;
}

bool SuperShuckie64::write_file(const std::filesystem::path &path, const std::vector<std::byte> &buffer) {
    auto path_cstr = path.string();
    auto *f = std::fopen(path_cstr.c_str(), "wb");

    if(!f) {
        DISPLAY_ERROR_DIALOG("Can't open file", "Can't open '%s' for writing!", path_cstr.c_str());
        return false;
    }

    int result = std::fwrite(buffer.data(), buffer.size(), 1, f);
    std::fclose(f);

    if(result != 1) {
        DISPLAY_ERROR_DIALOG("Can't write file", "Failed to write %zu to '%s'!", buffer.size(), path_cstr.c_str());
    }

    return result == 1;

}
