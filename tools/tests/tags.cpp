#include <cassert>
#include "../../source/library.hpp"

int main() {
    Library lib;
    lib.build({"sdmc:/Music"}, {"sdmc:/Music/01 Artist - Song.mp3", "sdmc:/Music/Band/Album/02 Song.mp3"});
    const auto original = lib.tracks;
    for (int i = 0; i < int(lib.tracks.size()); i++) {
        Track& track = lib.tracks[i];
        track.title = track.artist = track.album = track.albumArtist = track.genre = track.date = "override";
        track.trackNo = 99;
        track.tagged = true;
        track.albumIdx = 7;
        lib.resetTags(i);
        assert(track.path == original[i].path && track.folder == original[i].folder);
        assert(track.title == original[i].title && track.artist == original[i].artist && track.album == original[i].album);
        assert(track.albumArtist.empty() && track.genre.empty() && track.date.empty() && track.trackNo == 0);
        assert(track.tagged && track.albumIdx == 7);
    }
}
