#pragma once
#include <cstdint>
#include <list>
#include <map>
#include <unordered_map>

// Buy or sell. uint8_t so the Order struct stays small/packed.
enum class Side : uint8_t { Buy, Sell };

// One order. Prices are integer "cents": $100.55 -> 10055.
// We never use floats for prices (rounding bugs + slower compares).
struct Order {
    uint64_t id;
    Side     side;
    uint32_t price;   // limit price in cents
    uint32_t qty;     // remaining quantity (shrinks as it fills)
};

// All resting orders sitting at one price, oldest at the front.
// std::list because cancel has to erase from the MIDDLE in O(1)
// and keep every other order's iterator valid.
struct PriceLevel {
    std::list<Order> fifo;
};

class OrderBook {
public:
    void add_limit(const Order& o);   // matches, rests the leftover
    void add_market(Order o);         // matches, never rests the leftover
    void cancel(uint64_t id);         // pull a resting order out of the book

    uint64_t trade_count() const { return trades_; }
    uint64_t volume()      const { return volume_; }

private:
    // bids sorted HIGH->low  -> begin() is the best (highest) bid
    std::map<uint32_t, PriceLevel, std::greater<uint32_t>> bids_;
    // asks sorted low->high   -> begin() is the best (lowest)  ask
    std::map<uint32_t, PriceLevel> asks_;
    // id -> exact position in whatever list it lives in -> O(1) cancel
    std::unordered_map<uint64_t, std::list<Order>::iterator> idx_;

    uint64_t trades_ = 0;   // number of fills
    uint64_t volume_ = 0;   // total quantity traded

    // The one hot function. Templated only because bids_ and asks_ are
    // different types (different comparators); the logic is identical.
    template <class CrossBook, class RestBook>
    void match(Order o, CrossBook& cross, RestBook& rest, bool rest_remainder);
};
