// Host parser regression: clang++ -std=c++20 -I sysmodule/lib/helix-aac tools/tests/mp4.cpp -o /tmp/gf-mp4-test
// Run in a temporary folder; optionally pass a real AAC .m4a file to test its sample tables too.
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <vector>

static size_t liveBytes = 0, peakBytes = 0;
void* operator new(size_t n) {
    auto* p = static_cast<size_t*>(std::malloc(n + __STDCPP_DEFAULT_NEW_ALIGNMENT__));
    if (!p) throw std::bad_alloc();
    *p = n;
    liveBytes += n;
    if (liveBytes > peakBytes) peakBytes = liveBytes;
    return reinterpret_cast<char*>(p) + __STDCPP_DEFAULT_NEW_ALIGNMENT__;
}
void operator delete(void* p) noexcept {
    if (!p) return;
    auto* base = reinterpret_cast<size_t*>(static_cast<char*>(p) - __STDCPP_DEFAULT_NEW_ALIGNMENT__);
    liveBytes -= *base;
    std::free(base);
}
void operator delete(void* p, size_t) noexcept { operator delete(p); }

#include "../../sysmodule/source/decoder.cpp"
#include "../../sysmodule/source/dec_mp4.cpp"

// The codec is not under test: keep these checks independent of Helix's platform assembly.
extern "C" {
HAACDecoder AACInitDecoder() { return reinterpret_cast<void*>(1); }
void AACFreeDecoder(HAACDecoder) {}
int AACSetRawBlockParams(HAACDecoder, int, AACFrameInfo*) { return 0; }
int AACFlushCodec(HAACDecoder) { return 0; }
int AACDecode(HAACDecoder, unsigned char**, int*, short* out) { std::fill(out, out + 2048, 0); return 0; }
void AACGetLastFrameInfo(HAACDecoder, AACFrameInfo* f) { *f = {}; f->nChans = 2; f->outputSamps = 2048; f->sampRateOut = 44100; }
}
std::unique_ptr<Decoder> openMp3(const char*) { return nullptr; }
std::unique_ptr<Decoder> openFlac(const char*) { return nullptr; }
std::unique_ptr<Decoder> openWav(const char*) { return nullptr; }

using Bytes = std::vector<uint8_t>;
static void u32(Bytes& out, uint32_t n) { for (int s = 24; s >= 0; s -= 8) out.push_back(uint8_t(n >> s)); }
static Bytes box(const char* kind, const Bytes& payload) {
    Bytes out;
    u32(out, uint32_t(payload.size() + 8));
    out.insert(out.end(), kind, kind + 4);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}
static void reject(const Bytes& bytes) {
    const char* path = "mp4-malformed.m4a";
    { std::ofstream f(path, std::ios::binary); f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()); }
    const size_t before = liveBytes;
    peakBytes = before;
    assert(!openMp4(path));
    assert(peakBytes - before < 320 * 1024);
    assert(liveBytes == before);
    std::remove(path);
}
int main(int argc, char** argv) {
    for (const char* kind : { "stts", "stsc", "stsz", "stco", "co64" }) {
        Bytes p;
        u32(p, 0); // version and flags
        if (!std::strcmp(kind, "stsz")) u32(p, 0);
        u32(p, 1u << 22); // claimed count without entries
        reject(box(kind, p));
        p.clear();
        u32(p, 0);
        if (!std::strcmp(kind, "stsz")) u32(p, 0);
        const unsigned diskSize = !std::strcmp(kind, "stsz") || !std::strcmp(kind, "stco") ? 4 : !std::strcmp(kind, "stsc") ? 12 : 8;
        u32(p, 150000); // entries exist, but exceed the decoder's memory budget
        p.resize(p.size() + 150000 * diskSize);
        reject(box(kind, p));
    }
    Bytes combined;
    for (const char* kind : { "stts", "stsc", "co64" }) {
        Bytes entries;
        u32(entries, 0); u32(entries, 16384);
        entries.resize(entries.size() + 16384 * (!std::strcmp(kind, "stsc") ? 12 : 8));
        const Bytes part = box(kind, entries);
        combined.insert(combined.end(), part.begin(), part.end());
    }
    reject(combined); // individually small tables must still share one budget
    Bytes p;
    u32(p, 0); u32(p, 1); u32(p, 1); u32(p, 1024);
    Bytes twice = box("stts", p), again = twice;
    twice.insert(twice.end(), again.begin(), again.end());
    reject(twice);
    reject(Bytes{ 0,0,0,1, 'm','o','o','v' }); // missing extended size
    reject(Bytes{ 0,0,0,1, 'm','o','o','v', 255,255,255,255,255,255,255,255 }); // size overflow
    reject(Bytes{ 0,0,0,12, 's','t','s','z', 0,0,0,0 }); // truncated fixed fields
    if (argc > 1) {
        auto d = openMp4(argv[1]);
        assert(d && d->rate == 44100 && d->channels == 2);
        int16_t pcm[2048];
        assert(d->read(pcm, 1024) == 1024);
    }
    std::cout << "MP4 table bounds passed\n";
}
