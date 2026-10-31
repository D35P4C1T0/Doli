#include "../../source/ui.hpp"
#include <cassert>
#include <cstdio>

int main() {
    Ui ui;
    const Id play = uid("play"), next = uid("next"), other = uid("nearby"), seek = uid("seek");
    auto frame = [&] {
        ui.beginFrame(1.f / 60);
        ui.item(play, {100, 100, 64, 64}, 32, DefaultFocus);
        ui.item(other, {170, 100, 48, 48});
        ui.item(next, {240, 100, 64, 64});
        ui.item(seek, {100, 190, 204, 48}, 6, Slider | NoSideEntry);
        ui.link(play, 1, 0, next);
        ui.link(next, -1, 0, play);
        ui.link(play, 0, 1, seek);
        ui.link(seek, 0, -1, play);
        ui.endFrame();
    };
    ui.focusDefault();
    frame();
    assert(ui.focused(play) && ui.showFocusRing());
    frame();
    ui.navigate(1, 0);
    assert(ui.focused(next)); // Explicit transport links beat a nearby unrelated control.
    ui.navigate(-1, 0);
    ui.navigate(0, 1);
    assert(ui.focused(seek));
    ui.activate();
    frame();
    assert(ui.engaged(seek));
    ui.navigate(0, -1);
    assert(ui.focused(play) && !ui.anyEngaged());
    ui.touchDown(241, 101); // A tap near the corner still hits the full control.
    ui.touchUp(241, 101);
    ui.beginFrame(1.f / 60);
    assert(ui.item(next, {240, 100, 64, 64}));
    ui.endFrame();
    std::puts("UI controller/touch checks passed");
}
