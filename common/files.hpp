#pragma once

#include <cerrno>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#if defined(__NEWLIB__) && defined(__STRICT_ANSI__)
extern "C" int fileno(FILE*);   // newlib hides this POSIX declaration in the overlay's strict C++ mode
#endif

namespace files {

inline FILE* openRead(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    // FAT cannot replace a file by rename: the previous save survives in .bak during replacement.
    if (!f && errno == ENOENT) f = fopen((path + ".bak").c_str(), "rb");
    return f;
}

inline bool writeAtomic(const std::string& path, const std::string& data) {
    const std::string tmp = path + ".tmp", backup = path + ".bak";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    if (fflush(f) != 0) ok = false;
#ifdef _WIN32
    if (_commit(_fileno(f)) != 0) ok = false;
#else
    if (fsync(fileno(f)) != 0) ok = false;
#endif
    if (fclose(f) != 0) ok = false;
    if (!ok) { remove(tmp.c_str()); return false; }
    if (rename(tmp.c_str(), path.c_str()) != 0) {
        if (errno != EEXIST) return false;
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
#ifdef _WIN32
        if ((st.st_mode & _S_IFMT) != _S_IFREG) return false;
#else
        if (!S_ISREG(st.st_mode)) return false;
#endif
        if (remove(backup.c_str()) != 0 && errno != ENOENT) return false;
        if (rename(path.c_str(), backup.c_str()) != 0) return false;
        if (rename(tmp.c_str(), path.c_str()) != 0) {
            rename(backup.c_str(), path.c_str());
            return false;
        }
    }
    remove(backup.c_str());
    return true;
}

inline std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> out;
    FILE* f = openRead(path);
    if (!f) return out;
    std::string line;
    char buf[1024];
    while (fgets(buf, sizeof buf, f)) {
        line += buf;
        if (line.empty() || line.back() != '\n') continue;
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
        if (!line.empty()) out.push_back(std::move(line));
        line.clear();
    }
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty()) out.push_back(std::move(line));
    if (ferror(f)) out.clear();
    fclose(f);
    return out;
}

}  // namespace files
