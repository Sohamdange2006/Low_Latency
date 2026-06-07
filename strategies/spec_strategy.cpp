#include "strategy.hpp"
#include <vector>
#include <string_view>
#include <array>

class SpecStrategy : public csot::Strategy {
    struct SymbolState {
        std::string_view symbol;
        std::array<double, 64> mids{};
        double sum = 0.0;
        double sum_sq = 0.0;
        uint32_t count = 0;
        uint32_t head = 0;
        int32_t position = 0;
    };

    std::array<SymbolState, 64> states{};
    uint32_t num_symbols = 0;

    __attribute__((always_inline)) inline SymbolState& get_state(std::string_view sym) {
        const char* sym_ptr = sym.data(); 
        for (uint32_t i = 0; i < num_symbols; ++i) {
            if (states[i].symbol.data() == sym_ptr) [[likely]] {
                return states[i];
            }
        }
        states[num_symbols].symbol = sym;
        return states[num_symbols++];
    }

public:
    std::vector<csot::Order> on_tick(const csot::Tick& t) override {
        SymbolState& st = get_state(t.symbol);

        const double mid = (t.bid_px + t.ask_px) * 0.5;
        const double old_mid = st.mids[st.head];

        st.mids[st.head] = mid;
        st.head = (st.head + 1) & 63; 

        // FIXED WARMUP LOGIC
        if (st.count < 64) [[unlikely]] {
            st.count++;
            st.sum += mid;
            st.sum_sq += mid * mid;
            
            // We ONLY return empty if we STILL haven't hit 64
            if (st.count < 64) {
                return {}; 
            }
        } else {
            // O(1) Rolling Update only happens AFTER warmup
            st.sum += mid - old_mid;
            st.sum_sq += (mid * mid) - (old_mid * old_mid);
        }

        constexpr double INV_64 = 0.015625; 
        const double mean = st.sum * INV_64;
        const double variance = (st.sum_sq * INV_64) - (mean * mean);

        if (variance < 1e-18) [[unlikely]] return {};

        const double diff = mid - mean;
        const double diff_sq = diff * diff;

        if (st.position == 0) {
            if (diff_sq >= 4.0 * variance) {
                if (diff > 0.0) {
                    return {csot::Order{csot::Order::Side::SELL, t.symbol, t.bid_px, 1}};
                } else {
                    return {csot::Order{csot::Order::Side::BUY, t.symbol, t.ask_px, 1}};
                }
            }
            return {};
        }

        if (diff_sq <= 0.25 * variance) {
            if (st.position > 0) {
                return {csot::Order{csot::Order::Side::SELL, t.symbol, t.bid_px, static_cast<uint32_t>(st.position)}};
            } else {
                return {csot::Order{csot::Order::Side::BUY, t.symbol, t.ask_px, static_cast<uint32_t>(-st.position)}};
            }
        }

        return {};
    }

    void on_fill(const csot::Order& order, double price, uint32_t qty) override {
        SymbolState& st = get_state(order.symbol);
        if (order.side == csot::Order::Side::BUY) {
            st.position += qty;
        } else {
            st.position -= qty;
        }
    }
};

extern "C" csot::Strategy* create_strategy() {
    return new SpecStrategy();
}