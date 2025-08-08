#pragma once

#include <eosiolib/eosio.hpp>
#include <cmath>
#include "eosio.token.hpp"

/**
 * @class pollgf
 * EOSIO voting contract, supporting normal and token-weighted polls.
 */
class pollgf : public eosio::contract {
   public:
      typedef uint64_t                 poll_id_t;      // Type for poll IDs
      typedef std::vector<std::string> option_names_t; // List of option strings
      typedef eosio::extended_symbol   token_info_t;   // Token info (symbol+contract)
      typedef uint8_t                  option_id_t;    // Option index type

      pollgf(account_name contract_name)
         : eosio::contract(contract_name), _polls(contract_name, contract_name) {}

      /**
       * @struct option
       * Stores the name of a voting option.
       */
      struct option {
         std::string name;

         option(std::string name) : name(name) {}
         option() {}

         EOSLIB_SERIALIZE(option, (name))
      };

      /**
       * @struct option_result
       * Extends option by also tracking the number of votes.
       */
      struct option_result : option {
         double votes = 0; // Vote total (can be fractional for token-weighted polls)

         option_result(const std::string& name, uint64_t votes) : option(name), votes(votes) {}
         option_result(const std::string& name) : option_result(name, 0) {}
         option_result() {}

         EOSLIB_SERIALIZE(option_result, (name)(votes))
      };

      typedef std::vector<option_result> option_results;

      /**
       * @struct poll
       * Stores a poll (question, options, vote tallies, etc).
       */
      //@abi table
      struct poll {
         poll_id_t      id;            // Poll unique id
         std::string    question;      // Poll question text
         option_results results;       // Array of results (option name + vote tally)
         bool           is_token_poll = false; // True if poll is token-weighted
         token_info_t   token;         // Token info (if token-weighted)

         uint64_t primary_key() const { return id; }

         // Used for reverse order lookup/indexing (optional)
         uint64_t get_reverse_key() const { return ~id; }

         // Initializes poll object with all values and options.
         void set(poll_id_t id, const std::string& question,
                  const option_names_t& options, bool is_token_poll,
                  token_info_t token);

         EOSLIB_SERIALIZE(poll, (id)(question)(results)(is_token_poll)(token))
      };

      /**
       * @struct poll_vote
       * Stores a user's vote in a poll (per user per poll).
       */
      //@abi table votes
      struct poll_vote {
         poll_id_t   poll_id;    // The poll id this vote belongs to
         option_id_t option_id;  // Chosen option index

         uint64_t primary_key() const { return poll_id; }
         EOSLIB_SERIALIZE(poll_vote, (poll_id)(option_id))
      };

      // Table of polls, with a reverse index (not strictly needed)
      typedef eosio::multi_index<N(poll), poll,
         eosio::indexed_by<N(reverse),
            eosio::const_mem_fun<poll, uint64_t, &poll::get_reverse_key>
         >
      > poll_table;

      // Table of votes for each user (scope = user account)
      typedef eosio::multi_index<N(votes), poll_vote> vote_table;

      //@abi action
      void newpoll(const std::string& question, account_name creator,
                   const std::vector<std::string>& options);

      //@abi action
      void newtokenpoll(const std::string& question, account_name payer,
                        const std::vector<std::string>& options,
                        token_info_t token);

      //@abi action
      void vote(poll_id_t id, account_name voter, option_id_t option_id);

   private:
      // Stores poll on-chain.
      void store_poll(const std::string& question, account_name owner,
                      const option_names_t& options,
                      bool is_token_poll, token_info_t token);

      // Stores a user's vote and increments result.
      void store_vote(const poll& p, vote_table& votes, option_id_t option_id, double weight);

      // Stores a user's vote, using their token balance as weight.
      void store_token_vote(const poll& p, vote_table& votes, option_id_t option_id);

      // Converts an EOSIO asset to a weight (as a floating-point number)
      double to_weight(const eosio::asset& stake) {
         return stake.amount / std::pow(10, stake.symbol.precision());
      }

      poll_table _polls; // Main on-chain poll storage table
};