#pragma once

// ==========================================================
// node_arena.hpp
//
// NodeArena<Val, Ind>: the single growable linear buffer
// that backs the FLSSS iterative solver.
//
// The solver explores a branch-and-bound tree depth-first.
// Instead of real recursion, it keeps the live
// root-to-current path as a strict LIFO stack of "node"
// records, and stores that ENTIRE stack in ONE contiguous
// byte buffer. Pushing / popping a node is O(1)
// bump-pointer work, and there is ZERO per-node heap
// allocation.
//
// Node chunk layout
// -----------------
// From a node's max-aligned base offset `fb`:
//
//   [ NodeHdr ][ Min Max sumLB sumUB ][ LB  UB ]
//     header      SLOT_COUNT Val slots   2 Ind[]
//
// The element type is a trivially-copyable scalar Val.
//
// Growth
// ------
// Live chunks always form a gap-free prefix [0, top).
// When a push would overflow the buffer, relocate()
// allocates a larger block and moves every live chunk
// with a single memcpy per chunk (valid because Val is
// trivially copyable).
// ==========================================================

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <utility>

namespace flsss_detail {

// The four Val fields carried by every node, in slot order.
enum Slot : int {
    SLOT_MIN    = 0,   // running lower target bound
    SLOT_MAX    = 1,   // running upper target bound
    SLOT_SUM_LB = 2,   // sum of v[LB[i]] over the box
    SLOT_SUM_UB = 3,   // sum of v[UB[i]] over the box
    SLOT_COUNT  = 4
};

// Driver state of a node in the explicit DFS. A node is
// visited once per stage: Enter (tighten + first child),
// AfterA (second child), AfterB (pop). Stored as a
// 1-byte tag inside NodeHdr.
enum class Stage : uint8_t {
    Enter,
    AfterA,
    AfterB
};

// Per-node control block (a POD; carries no Val payload).
// Fields are ordered to minimize padding: the sole 8-byte
// offset first, then the two 1-byte tags, then the
// Ind-width fields.
template <typename Ind>
struct NodeHdr {
    size_t  parentBase;   // parent chunk offset (NONE at root)
    Stage        stage;        // driver state (1 byte)
    uint8_t lowerFirst;   // explore the lower half first?
    Ind          cap;          // array capacity at creation
    Ind          len;          // current len after collapse
    Ind          position;     // chosen branch position
    Ind          mid;          // branch split point
    Ind          appended;     // pinned indices on `hope`
};

// ----------------------------------------------------------
// The arena proper: owns the buffer, defines the chunk
// geometry, and exposes typed accessors into any live
// chunk.
// ----------------------------------------------------------
template <typename Val, typename Ind>
class NodeArena {
    static_assert(std::is_trivially_copyable_v<Val>,
                  "NodeArena assumes a trivially-copyable Val; "
                  "relocation moves chunks by memcpy.");
public:
    using Hdr = NodeHdr<Ind>;

    // Sentinel parent offset for the root node.
    static constexpr size_t NONE = size_t(-1);

    NodeArena() = default;
    NodeArena(const NodeArena&) = delete;
    NodeArena& operator=(const NodeArena&) = delete;
    ~NodeArena() { std::free(buffer_); }

    void swap(NodeArena& o) noexcept {
        std::swap(buffer_, o.buffer_);
        std::swap(cap_, o.cap_);
        std::swap(top_, o.top_);
    }
    void clear() noexcept { top_ = 0; }
    [[nodiscard]] size_t capacity() const { return cap_; }

    // -------- chunk geometry (sizes + cap) --------
    [[nodiscard]] static size_t alignUp(size_t x,
                                             size_t a) {
        return (x + (a - 1)) / a * a;
    }
    // Strongest alignment any region in a chunk needs.
    [[nodiscard]] static size_t maxAlign() {
        return std::max({alignof(Ind), alignof(Val), alignof(Hdr)});
    }
    // Offset of the Val slot array within a chunk.
    [[nodiscard]] static size_t slotsRel() {
        return alignUp(sizeof(Hdr), alignof(Val));
    }
    // Offset of the LB/UB index arrays.
    [[nodiscard]] static size_t arraysRel() {
        return alignUp(slotsRel() + SLOT_COUNT * sizeof(Val),
                       alignof(Ind));
    }
    // Total bytes for a chunk whose arrays hold `cap`
    // entries each.
    [[nodiscard]] static size_t chunkBytes(Ind cap) {
        return arraysRel() + 2 * cap * sizeof(Ind);
    }

    // -------- buffer lifetime / stack discipline --------
    // Ensures at least n bytes of capacity without
    // zero-filling unused storage beyond the live
    // [0, top_) prefix.
    void reserveBytes(size_t n) { ensureCapacity(n); }
    [[nodiscard]] size_t top() const { return top_; }

    // Reserve one chunk of capacity `cap`; returns its
    // base offset. May relocate the whole buffer
    // (invalidating raw pointers, so callers re-fetch
    // accessors after alloc()).
    [[nodiscard]] size_t alloc(Ind cap) {
        const auto A   = maxAlign();
        const auto fb  = alignUp(top_, A);
        const auto end = fb + chunkBytes(cap);
        if (end > cap_)
            ensureCapacity(std::max(end, cap_ * 2 + 256));
        top_ = end;
        return fb;
    }
    // Pop back to (and including nothing above) `base`.
    void freeTo(size_t base) { top_ = base; }

    // -------- typed accessors into a live chunk --------
    [[nodiscard]] Hdr* hdr(size_t fb) {
        return reinterpret_cast<Hdr*>(at(fb));
    }
    [[nodiscard]] Val* slot(size_t fb, int s) {
        return reinterpret_cast<Val*>(
            at(fb + slotsRel() + s * sizeof(Val)));
    }
    [[nodiscard]] Val* vMin(size_t fb) {
        return slot(fb, SLOT_MIN);
    }
    [[nodiscard]] Val* vMax(size_t fb) {
        return slot(fb, SLOT_MAX);
    }
    [[nodiscard]] Val* vSumLB(size_t fb) {
        return slot(fb, SLOT_SUM_LB);
    }
    [[nodiscard]] Val* vSumUB(size_t fb) {
        return slot(fb, SLOT_SUM_UB);
    }
    [[nodiscard]] Ind* lb(size_t fb) {
        return reinterpret_cast<Ind*>(at(fb + arraysRel()));
    }
    [[nodiscard]] Ind* ub(size_t fb, Ind cap) {
        return reinterpret_cast<Ind*>(
            at(fb + arraysRel() + cap * sizeof(Ind)));
    }

    // A bundle of pointers into one live chunk, gathered
    // in a single pass. WARNING: invalidated by the next
    // alloc()/relocate() -- the buffer may move, dangling
    // every pointer. Use only as a short-lived local
    // within a region that does NOT allocate.
    struct NodeView {
        Hdr* hdr;
        Val* min;
        Val* max;
        Val* sumLB;
        Val* sumUB;
        Ind* lb;
        Ind* ub;
    };
    [[nodiscard]] NodeView view(size_t fb) {
        auto* h = hdr(fb);
        return NodeView{h,
                        vMin(fb), vMax(fb), vSumLB(fb), vSumUB(fb),
                        lb(fb), ub(fb, h->cap)};
    }

    void bindSlot(size_t fb, int s, const Val& src) {
        *slot(fb, s) = src;
    }

private:
    [[nodiscard]] std::byte* at(size_t off) {
        return buffer_ + off;
    }

    void ensureCapacity(size_t need) {
        if (need <= cap_) return;
        const auto newCap = std::max(need, cap_ * 2 + 256);
        auto* nb = reinterpret_cast<std::byte*>(std::realloc(
            buffer_, newCap));
        if (!nb && newCap != 0) return;
        buffer_ = nb;
        cap_ = newCap;
    }

    std::byte* buffer_ = nullptr;
    size_t cap_ = 0;
    size_t top_ = 0;   // bump pointer (== live bytes)
};

}  // namespace flsss_detail
