#include "../../source/loader.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <optional>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace {
std::unordered_set<SDL_Surface*> freed;
}

// Count releases without initializing SDL or freeing the stack-owned test surfaces.
extern "C" void SDLCALL SDL_FreeSurface(SDL_Surface* surface) {
    assert(surface && freed.insert(surface).second);
}

int main() {
    using Result = TrackLoader::Result;
    static_assert(!std::is_copy_constructible_v<Result>);
    static_assert(!std::is_copy_assignable_v<Result>);
    static_assert(std::is_nothrow_move_constructible_v<Result>);
    static_assert(std::is_nothrow_move_assignable_v<Result>);

    std::array<SDL_Surface, 7> surfaces{};
    {
        Result worker{};
        worker.gen = 17;
        worker.info.title = "Loaded track";
        worker.cover.reset(&surfaces[0]);
        worker.blur.reset(&surfaces[1]);
        worker.frost.reset(&surfaces[2]);

        Result ready = std::move(worker);
        assert(!worker.cover && !worker.blur && !worker.frost);
        assert(freed.empty());

        Result pending{};
        pending.cover.reset(&surfaces[3]);
        pending = std::move(ready);
        assert(freed.size() == 1 && freed.contains(&surfaces[3]));
        assert(!ready.cover && !ready.blur && !ready.frost);
        assert(pending.gen == 17 && pending.info.title == "Loaded track");
        assert(pending.cover.get() == &surfaces[0]);
        assert(pending.blur.get() == &surfaces[1]);
        assert(pending.frost.get() == &surfaces[2]);
    }
    assert(freed.size() == 4); // Only the final owner releases the three transferred surfaces.

    {
        Result abandoned{};
        abandoned.cover.reset(&surfaces[4]);
    }
    assert(freed.size() == 5 && freed.contains(&surfaces[4]));

    std::optional<Result> pending;
    assert(!pending);
    pending.emplace();
    assert(pending && !pending->cover); // A result without art must still clear the previous cover.
    pending->cover.reset(&surfaces[5]);
    {
        Result upload = std::move(*pending);
        pending.reset();
        assert(!pending && !freed.contains(&surfaces[5]));
        assert(upload.cover.get() == &surfaces[5]);
    }
    assert(freed.size() == 6 && freed.contains(&surfaces[5]));

    pending.emplace();
    pending->blur.reset(&surfaces[6]);
    pending.reset(); // Shutdown can discard an upload that has not reached the next frame.
    assert(freed.size() == 7 && freed.contains(&surfaces[6]));
    std::puts("Track loader ownership checks passed");
}
