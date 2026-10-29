// Run from a disposable directory: this creates a fake sdmc: tree.
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace {
std::atomic<bool> fat = false, failPromote = false, failRollback = false;
std::atomic<bool> shortWrite = false, failClose = false, failSync = false, failRename = false;
std::mutex gate;
std::condition_variable wake;
std::string blocked;
bool waiting = false, released = false;

FILE* testOpen(const char* path, const char* mode) {
    std::unique_lock<std::mutex> lock(gate);
    if (path == blocked && !released) {
        waiting = true;
        wake.notify_all();
        wake.wait(lock, [] { return released; });
    }
    return fopen(path, mode);
}

int testRename(const char* from, const char* to) {
    if (fat && std::filesystem::exists(to)) { errno = EEXIST; return -1; }
    if (failRename || (failPromote && std::string(from).ends_with(".tmp")) ||
        (failRollback && std::string(from).ends_with(".bak"))) { errno = EIO; return -1; }
    return rename(from, to);
}

size_t testWrite(const void* data, size_t size, size_t n, FILE* f) {
    return fwrite(data, size, shortWrite && n ? n - 1 : n, f);
}

int testClose(FILE* f) {
    const int result = fclose(f);
    return failClose ? EOF : result;
}

int testSync(int fd) { return failSync ? -1 : fsync(fd); }
}

#define fopen testOpen
#define rename testRename
#define fwrite testWrite
#define fclose testClose
#define fsync testSync
#include "../../source/store.cpp"
#undef fopen
#undef rename
#undef fwrite
#undef fclose
#undef fsync

static std::string read(const std::string& path) {
    FILE* f = files::openRead(path);
    assert(f);
    std::string out;
    char buf[1024];
    for (size_t n; (n = fread(buf, 1, sizeof buf, f));) out.append(buf, n);
    fclose(f);
    return out;
}

int main() {
    std::filesystem::create_directories(Store::Dir);
    Store store;
    store.subsonic = { true, "https://music.example.com/base", "user & café", "secret=+&é" };
    store.save();
    Store::flush();
    Store restored;
    restored.load();
    assert(restored.subsonic.enabled && restored.subsonic.url == store.subsonic.url);
    assert(restored.subsonic.user == store.subsonic.user && restored.subsonic.password == store.subsonic.password);
    Session saved, loaded;
    saved.pos = 2500;
    saved.position = 42.5;
    for (int i = 0; i < 3000; i++) {
        saved.tracks.push_back("sdmc:/Music/" + std::to_string(i) + ".mp3");
        saved.order.push_back(2999 - i);
    }
    store.saveSession(saved);
    Store::flush();
    assert(store.loadSession(loaded));
    assert(loaded.tracks == saved.tracks && loaded.order == saved.order);
    assert(loaded.pos == saved.pos && loaded.position == saved.position);
    assert(files::writeAtomic("lines", "\r\n" + std::string(12000, 'x') + "\r\nlast"));
    assert((files::readLines("lines") == std::vector<std::string>{std::string(12000, 'x'), "last"}));

    assert(files::writeAtomic("save", "old"));
    for (std::atomic<bool>* fault : {&shortWrite, &failSync, &failClose, &failRename}) {
        *fault = true;
        assert(!files::writeAtomic("save", "new"));
        *fault = false;
        assert(read("save") == "old");
    }
    fat = true;
    assert(files::writeAtomic("save", "new"));
    assert(read("save") == "new");
    failPromote = true;
    assert(!files::writeAtomic("save", "next"));
    assert(read("save") == "new");
    failRollback = true;
    assert(!files::writeAtomic("save", "next"));
    assert(!std::filesystem::exists("save"));
    assert(read("save") == "new");   // interrupted FAT promotion: the backup remains readable
    failPromote = failRollback = false;
    assert(files::writeAtomic("save", "recovered"));
    assert(read("save") == "recovered");
    assert(!std::filesystem::exists("save.bak"));
    fat = false;

    store.writeFile("retry", "old");
    Store::flush();
    failRename = true;
    store.writeFile("retry", "new");
    Store::flush();
    failRename = false;
    assert(read(store.path("retry")) == "old");
    store.writeFile("retry", "new");
    Store::flush();
    assert(read(store.path("retry")) == "new");

    failRename = true;
    store.writeFile("retry", "automatic retry");
    Store::flush();
    failRename = false;
    for (int i = 0; i < 100 && read(store.path("retry")) != "automatic retry"; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(read(store.path("retry")) == "automatic retry");

    store.writeFile("race", "B");
    Store::flush();
    {
        std::lock_guard<std::mutex> lock(gate);
        blocked = store.path("race.tmp");
    }
    store.writeFile("race", "A");
    {
        std::unique_lock<std::mutex> lock(gate);
        assert(wake.wait_for(lock, std::chrono::seconds(5), [] { return waiting; }));
    }
    store.writeFile("race", "B");
    {
        std::lock_guard<std::mutex> lock(gate);
        released = true;
    }
    wake.notify_all();
    Store::flush();
    assert(read(store.path("race")) == "B");
    puts("storage checks passed");
}
