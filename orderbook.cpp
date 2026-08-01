#include "orderbook.hpp"
#include <algorithm>   // std::min
#include <limits>

// the hot path: match an incoming order against the opposite book
//
// `cross` is the book we eat into (asks if we're buying, bids if selling).
// `rest`  is the book the leftover sits in (our own side).
// A trade happens whenever buy_price >= sell_price. We compute which of
// the two is the buy/sell price from the incoming side, so one branch-free
// condition covers both directions.
template <class CrossBook, class RestBook>
void OrderBook::match(Order o, CrossBook& cross, RestBook& rest,
                      bool rest_remainder) {
    const bool buy = (o.side == Side::Buy);

    while (o.qty > 0 && !cross.empty()) {
        auto best = cross.begin();          // best price level on the far side
        uint32_t rp = best->first;          // chase #1: walk into RB-tree

        uint32_t bid_px = buy ? o.price : rp;
        uint32_t ask_px = buy ? rp : o.price;
        if (bid_px < ask_px) break;         // best level no longer crosses

        auto& q = best->second.fifo;        // FIFO at this price (time priority)
        while (o.qty > 0 && !q.empty()) {
            Order& maker = q.front();       //  chase #2: load the list node
            uint32_t fill = std::min(o.qty, maker.qty);

            o.qty     -= fill;
            maker.qty -= fill;
            ++trades_;
            volume_ += fill;

            if (maker.qty == 0) {           // resting order fully consumed
                idx_.erase(maker.id);       //  chase #3: hash erase
                q.pop_front();              // chase #4: free + relink node
            }
        }
        if (q.empty()) cross.erase(best);   //  chase #5: RB-tree rebalance
    }

    // leftover limit qty rests; market leftover is thrown away
    if (rest_remainder && o.qty > 0) {
        auto& lvl = rest[o.price];          // creates the level if new
        idx_[o.id] = lvl.fifo.insert(lvl.fifo.end(), o);
    }
}

void OrderBook::add_limit(const Order& o) {
    if (o.side == Side::Buy) match(o, asks_, bids_, true);
    else                     match(o, bids_, asks_, true);
}

void OrderBook::add_market(Order o) {
    // A market order crosses at ANY price, so give it the most aggressive
    // price possible and then refuse to rest the remainder.
    o.price = (o.side == Side::Buy) ? std::numeric_limits<uint32_t>::max() : 0;
    if (o.side == Side::Buy) match(o, asks_, bids_, false);
    else                     match(o, bids_, asks_, false);
}

void OrderBook::cancel(uint64_t id) {
    auto it = idx_.find(id);
    if (it == idx_.end()) return;           // already filled / never existed -> no-op

    auto list_it = it->second;              // iterator straight to the node
    uint32_t price = list_it->price;
    Side     side  = list_it->side;

    if (side == Side::Buy) {
        auto lvl = bids_.find(price);
        lvl->second.fifo.erase(list_it);    // O(1) middle erase, this is the point
        if (lvl->second.fifo.empty()) bids_.erase(lvl);
    } else {
        auto lvl = asks_.find(price);
        lvl->second.fifo.erase(list_it);
        if (lvl->second.fifo.empty()) asks_.erase(lvl);
    }
    idx_.erase(it);
}
