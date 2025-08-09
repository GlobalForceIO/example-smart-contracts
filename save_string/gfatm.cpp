#include "gfatm.hpp"

/**
 * Action: set configuration.
 * For fixed 10,000 GFT/day logic, this does nothing. Only here for ABI compatibility.
 */
void gfatm::config(int8_t timezone, uint64_t daily_limit)
{
    // Not implemented (static logic).
    // To implement dynamic config, use require_auth(_self) and a singleton.
}

/**
 * @brief Handler for incoming transfers (typically called via inline transfer).
 * If sender is not self (i.e., an external user is withdrawing from ATM), enforce daily withdrawal limit.
 * Only GFT tokens from gft.token contract are processed—others are ignored.
 */
void gfatm::handle_transfer(account_name from, account_name to, extended_asset quantity, string memo)
{
    // Ignore outgoing transfers or non-GFT tokens
    if (from == _self) return;

    // Only enforce for GFT token withdrawals
    if (quantity.contract != GFT_TOKEN_CONTRACT || quantity.quantity.symbol != GFT_SYMBOL)
        return;

    // Ignore zero or negative transfers
    if (quantity.quantity.amount <= 0)
        return;

    // Get current UTC day number
    uint32_t today = current_day();

    // Open withdrawals table in contract scope
    withdrawals_table wtable(_self, _self.value);

    auto witr = wtable.find(from);
    uint64_t already_withdrawn = 0;
    uint32_t user_day = 0;

    if (witr != wtable.end()) {
        already_withdrawn = witr->amount_withdrawn;
        user_day = witr->last_withdraw_day;
    }

    // If it's a new day, reset user's withdrawn amount
    if (witr == wtable.end() || user_day != today) {
        already_withdrawn = 0;
    }

    uint64_t new_total = already_withdrawn + quantity.quantity.amount;

    // Enforce per-day withdrawal limit
    check(new_total <= DAILY_LIMIT, "Daily withdrawal limit of 10,000 GFT exceeded for this account");

    // Record/update withdrawal
    if (witr == wtable.end()) {
        wtable.emplace(_self, [&](auto& row) {
            row.account = from;
            row.amount_withdrawn = quantity.quantity.amount;
            row.last_withdraw_day = today;
        });
    } else if (user_day != today) {
        wtable.modify(witr, _self, [&](auto& row) {
            row.amount_withdrawn = quantity.quantity.amount;
            row.last_withdraw_day = today;
        });
    } else {
        wtable.modify(witr, _self, [&](auto& row) {
            row.amount_withdrawn = new_total;
            // last_withdraw_day remains the same
        });
    }

    // Process the withdrawal as normal (i.e., transfer will succeed)
    // Any payout logic or actual token release should happen here if needed.

    // NOTE: If excess, check() above will revert and the withdrawal will fail.
}