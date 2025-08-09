#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/singleton.hpp>
#include <string>

using namespace std;
using namespace eosio;

/**
 * Structure for unpacking data from transfer (from standard GF token).
 */
struct transfer_args
{
    account_name from;     // Sender account
    account_name to;       // Recipient account
    asset quantity;        // Amount of tokens
    string memo;           // Arbitrary memo/message
};

/**
 * Main contract class: GF ATM.
 * This contract is intended to service token operations (for example, withdrawal limits and working timezone).
 */
class gfatm : public contract
{
  public:
    using contract::contract;

    // The token symbol and contract for which operations are limited (by default, GFT)
    const extended_symbol LIMITING_TOKEN = extended_symbol(S(4, GFT), N(eosio.token));

    /**
     * Structure for storing contract configuration:
     * - timezone: working timezone of the ATM (for example, to restrict operation by time of day)
     * - daily_limit: daily withdrawal limit (in token units)
     * Used as a singleton — only one config record per contract.
     */
    struct config_info
    {
        int8_t timezone;         // Time zone offset (e.g., GMT+3 = 3)
        uint64_t daily_limit;    // Daily limit (e.g., 10000 = 1 GFT in 4-decimal format)
    };

    // Singleton to store the configuration
    typedef singleton<N(config), config_info> tbl_config;

    /**
     * Action: set the configuration (timezone and limit).
     * Can be called only by the contract owner.
     */
    void config(int8_t timezone, uint64_t daily_limit);

    /**
     * Handler for incoming transfers.
     * Used for accounting, enforcing limits, etc.
     * from — sender, to — recipient, quantity — amount, memo — arbitrary message.
     */
    void handle_transfer(account_name from, account_name to, extended_asset quantity, string memo);
};

/**
 * ABI section (C-style apply), necessary for correct GF action routing.
 * - If action's code matches the contract — dispatch actions via GF_API.
 * - If a standard transfer is received from another contract (e.g., eosio.token) — call handle_transfer.
 */
extern "C"
{
    void apply(uint64_t receiver, uint64_t code, uint64_t action)
    {
        auto self = receiver;
        gfatm thiscontract(self);

        // Action call (e.g., config)
        if (code == self)
        {
            switch (action)
            {
                EOSIO_API(gfatm, (config))
            }
        }
        // Handling incoming transfer
        else if (action == N(transfer))
        {
            // Unpack transfer arguments
            auto transfer_data = unpack_action_data<transfer_args>();
            thiscontract.handle_transfer(
                transfer_data.from,
                transfer_data.to,
                extended_asset(transfer_data.quantity, code),
                transfer_data.memo
            );
        }
    }
}
