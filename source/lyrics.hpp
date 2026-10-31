#pragma once
#include <string>
#include <vector>

struct LyricLine { double t; std::string text; };

struct Lyrics {
    std::vector<LyricLine> lines;
    bool synced = false;

    static Lyrics parse(const std::string& text);
};
