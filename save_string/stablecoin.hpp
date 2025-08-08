#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

using namespace eosio;
using std::string;

/**
 * The stablecoin contract is an implementation of its own token with standard capabilities,
 * as well as support for a blacklist of accounts and contract pause.
 */
CONTRACT stablecoin : public contract {
public:
      using contract::contract;

      /**
	   * Token creation.
	   * issuer — issuer (token owner, who has the right to issue).
	   * maximum_supply — maximum token issue volume.
       */
      ACTION create( name issuer, asset maximum_supply );

      /**
       * Token emission (mint).
	   * to — recipient account.
	   * quantity — quantity of tokens.
	   * memo — arbitrary comment.
       */
      ACTION issue( name to, asset quantity, string memo );

      /**
       * Transfer tokens between accounts.
	   * from — sender.
	   * to — recipient.
	   * quantity — quantity.
	   * memo — arbitrary comment.
       */
      ACTION transfer( name from, name to, asset quantity, string memo );

      /**
	   * Token burning by the issuer.
	   * quantity — the number of tokens to be destroyed.
	   * memo — an arbitrary comment.
       */
      ACTION burn( asset quantity, string memo );

      /**
	   * Pause the contract.
	   * All transfers are blocked (except the pause/unpause method itself).
	   * Only the contract account can call.
       */
      ACTION pause();

      /**
	   * Unpause contract (allow transfers).
	   * Only contract account can call.
       */
      ACTION unpause();

      /**
	   * Adding an account to the blacklist.
	   * An account in the blacklist cannot send/receive tokens.
	   * memo — the reason for blocking.
       */
      ACTION blacklist( name account, string memo );

      /**
       * Removing an account from the blacklist.
       */
      ACTION unblacklist( name account );

      /**
       * Get the current supply (emission) of the token.
       */
      static asset get_supply( name token_contract_account,  symbol_code sym ) {
            stats statstable( token_contract_account, sym.raw() );
            const auto& st = statstable.get( sym.raw() );
            return st.supply;
      }

      /**
       * Get the balance of a specific account using the token symbol code.
       */
      static asset get_balance( name token_contract_account,  name owner, symbol_code sym ) {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym.raw() );
            return ac.balance;
      }

private:
      /**
	   * Account balance table.
	   * Each account and token has a balance stored.
       */
      TABLE account {
            asset       balance; // Balance in this token
            uint64_t primary_key()const { return balance.symbol.code().raw(); }
      };

      /**
	   * Table with information about each token (emission statistics).
	   * Includes supply, max_supply, issuer.
       */
      TABLE currency_stats {
            asset       supply;     // Current offer
            asset       max_supply; // Maximum permitted emission
            name        issuer;     // Token issuer
            uint64_t primary_key()const { return supply.symbol.code().raw(); }
      };

      /**
	   * Account blacklist table.
	   * Accounts from here cannot perform operations with the token.
       */
      TABLE blacklist_table {
            name      account; // Blocked account
            auto primary_key() const {  return account.value;  }
      };

      /**
       * Table for storing the contract pause status.
       */
      TABLE pause_table {
            uint64_t            id;     // Always 1, single line
            bool                paused; // True if the contract is paused
            auto primary_key() const {  return id;  }
      };

      // Definitions of multi_index tables for access within a contract
      typedef eosio::multi_index< "accounts"_n, account > accounts;
      typedef eosio::multi_index< "stat"_n, currency_stats > stats;
      typedef eosio::multi_index< "blacklists"_n, blacklist_table > blacklists;
      typedef eosio::multi_index< "pausetable"_n, pause_table > pausetable;

      /**
       * Internal method: decrease account balance (called on transfers/burning).
       */
      void sub_balance( name owner, asset value );

      /**
       * Internal method: increase account balance (called during transfers/issues).
       */
      void add_balance( name owner, asset value, name ram_payer );

      /**
       * Internal method: Check if the contract is in paused state.
       */
      bool is_paused();
};
