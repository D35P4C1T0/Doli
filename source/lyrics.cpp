#include "lyrics.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

/** [mm:ss.xx] LRC lines become timed lines; anything else is plain text. */
Lyrics Lyrics::parse(const std::string& text) {
    Lyrics result;
    double offset = 0;
    size_t start = 0;
    while (start <= text.size()) {
        size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        std::string line = text.substr(start, end - start);
        start = end + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::vector<double> stamps;
        size_t p = 0;
        while (p < line.size() && line[p] == '[') {
            const size_t close = line.find(']', p);
            if (close == std::string::npos) break;
            int mm = 0;
            double ss = 0;
            if (sscanf(line.c_str() + p + 1, "%d:%lf", &mm, &ss) == 2) stamps.push_back(mm * 60 + ss);
            else if (!stamps.empty()) break;
            else if (!line.compare(p + 1, 7, "offset:")) offset = atof(line.c_str() + p + 8) / 1000;   // ms; + is sooner
            p = close + 1;
        }
        std::string body = line.substr(p);
        while (!body.empty() && body.front() == ' ') body.erase(0, 1);
        if (!stamps.empty()) {
            result.synced = true;
            for (double t : stamps) result.lines.push_back({ t, body });
        } else if (p == 0) {
            result.lines.push_back({ -1, body });
        }
        if (end == text.size()) break;
    }
    if (result.synced) {
        std::erase_if(result.lines, [](const LyricLine& l) { return l.t < 0; });
        // [offset:] as LRC defines it: a positive offset shows every line that much sooner
        if (offset != 0) for (LyricLine& l : result.lines) l.t = std::max(0.0, l.t - offset);
        std::stable_sort(result.lines.begin(), result.lines.end(), [](const LyricLine& a, const LyricLine& b) { return a.t < b.t; });
    }
    while (!result.lines.empty() && result.lines.back().text.empty() && !result.synced) result.lines.pop_back();
    return result;
}
