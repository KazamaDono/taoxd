// ch17-uaf-typeconfusion/src/groom.hpp
// Header-only helpers to drain and pre-populate the glibc 2.39 tcache 0x30
// bin, used by both labs when a deterministic reclaim is required.
//
// The tcache is per-thread, per-size, LIFO, with a default cap of 7 chunks
// per bin. Draining puts the bin in a known-empty state so the next free
// lands at the head; pre-populating stacks decoys so the target chunk lands
// at a chosen depth.

#ifndef CH17_GROOM_HPP
#define CH17_GROOM_HPP

#include <cstddef>
#include <cstdlib>
#include <vector>

namespace groom {

// Drain a size-class by pulling and holding N chunks so the bin is empty
// afterwards (caller frees them when they want to release).
inline std::vector<void*> drain(size_t sz, int n = 7) {
    std::vector<void*> v; v.reserve(n);
    for (int i = 0; i < n; ++i) v.push_back(std::malloc(sz));
    return v;
}

// Free `v` back into the tcache (LIFO), returning it to a filled state.
inline void refill(std::vector<void*> &v) {
    for (auto *p : v) std::free(p);
    v.clear();
}

// Pre-seed the tcache with `depth` freed chunks so the next free lands
// at depth+1; use when a program allocates before you can drive it.
inline void seed(size_t sz, int depth) {
    std::vector<void*> tmp; tmp.reserve(depth);
    for (int i = 0; i < depth; ++i) tmp.push_back(std::malloc(sz));
    for (auto *p : tmp) std::free(p);
}

} // namespace groom

#endif // CH17_GROOM_HPP
