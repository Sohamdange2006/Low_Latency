#include "orderbook.hpp"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// One parsed line of the CSV, before we hand it to the book.
//   action: 'N' new limit, 'M' market, 'C' cancel
struct Row {
    char     action;
    uint64_t id;
    Side     side;
    uint32_t price;
    uint32_t qty;
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " orders.csv\n";
        return 1;
    }

    // --- LOAD PHASE (not timed) --------------------------------------------
    // Pull the whole file into a vector first. Disk I/O and parsing have
    // nothing to do with matching speed, so we keep them out of the clock.
    std::ifstream in(argv[1]);
    if (!in) { std::cerr << "cannot open " << argv[1] << "\n"; return 1; }

    std::vector<Row> rows;
    rows.reserve(1'000'000);

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        unsigned long long id = 0, price = 0, qty = 0;
        char act = 0, side_c = 0;
        // action,id,side,price,qty
        std::sscanf(line.c_str(), "%c,%llu,%c,%llu,%llu",
                    &act, &id, &side_c, &price, &qty);
        Row r;
        r.action = act;
        r.id     = id;
        r.side   = (side_c == 'B') ? Side::Buy : Side::Sell;
        r.price  = static_cast<uint32_t>(price);
        r.qty    = static_cast<uint32_t>(qty);
        rows.push_back(r);
    }
    std::cerr << "loaded " << rows.size() << " orders\n";

    // --- MATCH PHASE (timed) -----------------------------------------------
    OrderBook book;
    auto t0 = std::chrono::steady_clock::now();

    for (const Row& r : rows) {
        Order o{r.id, r.side, r.price, r.qty};
        switch (r.action) {
            case 'N': book.add_limit(o);  break;
            case 'M': book.add_market(o); break;
            case 'C': book.cancel(r.id);  break;
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();

    std::cerr << "trades: " << book.trade_count()
              << "   volume: " << book.volume() << "\n";
    std::fprintf(stderr, "matched %zu orders in %.4f s  =  %.2f M orders/sec\n",
                 rows.size(), secs, (rows.size() / secs) / 1e6);
    return 0;
}
