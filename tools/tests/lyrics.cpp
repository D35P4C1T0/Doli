#include "../../source/lyrics.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    const Lyrics empty = Lyrics::parse("");
    assert(empty.lines.empty() && !empty.synced);

    const Lyrics plain = Lyrics::parse("\r\n  First\r\n\r\nSecond\r\n\r\n");
    assert(!plain.synced && plain.lines.size() == 4);
    assert(plain.lines[0].text.empty() && plain.lines[1].text == "First");
    assert(plain.lines[2].text.empty() && plain.lines[3].text == "Second");
    for (const LyricLine& line : plain.lines) assert(line.t == -1);

    const Lyrics timed = Lyrics::parse(
        "[ar:Artist]\nIgnored plain line\n[00:10.00][00:02.00] Repeated\n"
        "[00:02.00] Equal timestamp\n[offset:500]\n[00:00.25]\n");
    assert(timed.synced && timed.lines.size() == 4);
    assert(timed.lines[0].t == 0 && timed.lines[0].text.empty());
    assert(timed.lines[1].t == 1.5 && timed.lines[1].text == "Repeated");
    assert(timed.lines[2].t == 1.5 && timed.lines[2].text == "Equal timestamp");
    assert(timed.lines[3].t == 9.5 && timed.lines[3].text == "Repeated");

    const Lyrics delayed = Lyrics::parse("[offset:-1250]\n[01:02.50]Later");
    assert(delayed.synced && delayed.lines.size() == 1);
    assert(std::abs(delayed.lines[0].t - 63.75) < 0.0001);

    const Lyrics unfinished = Lyrics::parse("[unfinished lyric");
    assert(!unfinished.synced && unfinished.lines.size() == 1);
    assert(unfinished.lines[0].text == "[unfinished lyric");

    const Lyrics section = Lyrics::parse("[00:01.00][Chorus]Words");
    assert(section.synced && section.lines[0].text == "[Chorus]Words");

    Lyrics outgoing = timed;
    outgoing.lines[1].text = "Outgoing song";
    assert(timed.lines[1].text == "Repeated");
    std::puts("Lyrics parsing checks passed");
}
