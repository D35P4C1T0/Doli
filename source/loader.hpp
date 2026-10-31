/*
 * Gatefold NX · loader.hpp
 * What a track change needs from the SD card and the CPU, done on a worker core: the tags and
 * format, the lyrics, the cover (your own, the embedded one or the folder's), decoded and shrunk,
 * its palette and its backdrop blur. The main thread only uploads the textures, so a track change
 * no longer stops the frame it happens in.
 */
#pragma once
#include <SDL2/SDL.h>

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "metadata.hpp"
#include "palette.hpp"

class TrackLoader {
public:
    using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;

    struct Request {
        unsigned gen = 0;
        std::string path, folder;
        std::string myCover;    // your cover for it (Store::covers), else empty
        bool readInfo = true;   // false: only the cover again (one set or forgotten in the track menu)
        PaletteOpts opts;
    };
    struct Result {
        unsigned gen = 0;
        bool readInfo = true;
        TrackInfo info;                  // without the cover bytes
        std::string lyrics;              // the .lrc file's, else the tag's
        bool sidecar = false;            // the lyrics came from an .lrc file
        Surface cover{ nullptr, SDL_FreeSurface };   // RGBA32, at most 512 px (Gfx::shrink)
        Surface blur{ nullptr, SDL_FreeSurface };    // 12×12, stretched with linear filtering for the backdrop
        Surface frost{ nullptr, SDL_FreeSurface };   // 64×64, blurred (three box passes): Automix's frost
        std::vector<uint8_t> px;         // the 64×64 sample the palette came from (Brightness and Saturation rebuild it)
        Palette pal;
    };

    void start();
    void stop();
    /** Replaces whatever is waiting: only the newest track matters. */
    void request(Request r);
    /** Moves the newest finished result to the caller, once, including ownership of its surfaces. */
    bool take(Result& out);

private:
    void run();
    static void work(const Request& r, Result& out);

    std::mutex m_;
    std::condition_variable cv_;
    std::thread thread_;
    bool quit_ = false, hasReq_ = false, hasDone_ = false;
    Request req_;
    Result done_;
};
