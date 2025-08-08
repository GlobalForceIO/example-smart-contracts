/**
 * polleos - A simple on-chain voting (poll) smart contract for EOSIO.
 * 
 * This contract allows anyone to create polls with multiple options, and for users to vote.
 * Polls can be standard (1 account = 1 vote) or "token-weighted" (vote weight is based on user's token balance).
 * Votes and poll data are stored on-chain for auditability and transparency.
 */

#include "pollgf.hpp"
#include <limits>

/**
 * @brief Sets up a poll object with the specified options and properties.
 * @param id            The poll's unique ID.
 * @param question      The poll question.
 * @param options       The list of options for voting.
 * @param is_token_poll True if this is a token-weighted poll.
 * @param token         Information about the token for weighting, if needed.
 */
void pollgf::poll::set(pollgf::poll_id_t id, const std::string& question,
                        const option_names_t& options, bool is_token_poll,
                        token_info_t token) {

   eosio_assert(!question.empty(), "Question can't be empty");

   this->id            = id;
   this->question      = question;
   this->is_token_poll = is_token_poll;
   this->token         = token;

   // Prepare results array for each voting option.
   results.resize(options.size());
   std::transform(options.begin(), options.end(), results.begin(),
                  [&](std::string str) {
                     eosio_assert(!str.empty(), "Option names can't be empty");
                     return option_result(str);
                  });
}

/**
 * @brief Stores a new poll in the contract's poll table.
 * @param question      The poll question.
 * @param poll_owner    Who pays RAM for this poll (creator).
 * @param options       The list of voting options.
 * @param is_token_poll Whether the poll is token-weighted.
 * @param token         Token info, if needed.
 */
void pollgf::store_poll(const std::string& question, account_name poll_owner,
                         const option_names_t& options,
                         bool is_token_poll, token_info_t token) {

   poll_id_t  id;

   eosio_assert(options.size() < std::numeric_limits<option_id_t>::max(),
                "Too many options");

   _polls.emplace(poll_owner, [&](poll& p) {
      id = _polls.available_primary_key();
      p.set(id, question, options, is_token_poll, token);
   });

   eosio::print("Poll stored with id: ", id);
}

/**
 * @brief Stores a user's vote in a poll (with explicit vote weight).
 *        Also increments the selected option's result.
 * @param p         The poll object.
 * @param votes     The vote table for the voter.
 * @param option_id The selected option's ID.
 * @param weight    The weight of the vote (1 for normal, token balance for token polls).
 */
void pollgf::store_vote(const pollgf::poll& p, pollgf::vote_table& votes,
                         option_id_t option_id, double weight) {

   eosio_assert(weight > 0, "Vote weight cannot be less than 0. Contract logic issue");

   // Voter (votes.get_scope()) pays for RAM.
   votes.emplace(votes.get_scope(), [&](poll_vote& v) {
      v.poll_id    = p.id;
      v.option_id  = option_id;
   });

   _polls.modify(p, votes.get_scope(), [&](poll& p) {
      p.results[option_id].votes += weight;
   });
}

/**
 * @brief Stores a user's token-weighted vote in a poll.
 *        Checks token balance, then records vote.
 * @param p         The poll object.
 * @param votes     The vote table for the voter.
 * @param option_id The selected option's ID.
 */
void pollgf::store_token_vote(const pollgf::poll& p, pollgf::vote_table& votes,
                               option_id_t option_id) {

   account_name voter = votes.get_scope();

   eosio::token token(p.token.contract);
   // Will fail if voter has no tokens
   eosio::asset balance = token.get_balance(voter, p.token.name());

   // Validate token balance
   eosio_assert(balance.is_valid(), "Balance of voter account is invalid. Something is wrong with token contract.");
   eosio_assert(balance.amount > 0, "Voter must have more than 0 tokens to participate in a poll!");

   // Store vote with token balance as weight
   store_vote(p, votes, option_id, to_weight(balance));
}

/**
 * @brief Create a new standard (non-token-weighted) poll.
 * @param question The poll question.
 * @param payer    Account paying for RAM.
 * @param options  The list of voting options.
 * @abi action
 */
void pollgf::newpoll(const std::string& question, account_name payer,
                      const option_names_t& options) {

   store_poll(question, payer, options, false, token_info_t());
}

/**
 * @brief Create a new token-weighted poll.
 * @param question   The poll question.
 * @param owner      Account paying for RAM.
 * @param options    The list of voting options.
 * @param token_inf  Info about the token to use for vote weighting.
 * @abi action
 */
void pollgf::newtokenpoll(const std::string& question, account_name owner,
                           const option_names_t& options, token_info_t token_inf) {

   eosio::token token(token_inf.contract);
   eosio_assert(token.exists(token_inf.name()), "This token does not exist");
   store_poll(question, owner, options, true, token_inf);
}

/**
 * @brief Cast a vote in a poll.
 *        Checks for double-voting and option validity, then stores the vote.
 * @param id        Poll id.
 * @param voter     Voter's account.
 * @param option_id Chosen option's index.
 * @abi action
 */
void pollgf::vote(pollgf::poll_id_t id, account_name voter, option_id_t option_id) {

   eosio::require_auth(voter);

   const poll & p = _polls.get(id, "Poll with this id does not exist");

   eosio_assert(option_id < p.results.size(), "Option with this id does not exist");

   vote_table votes(get_self(), voter);
   eosio_assert(votes.find(p.id) == votes.end(), "This account has already voted in this poll");

   if (p.is_token_poll)
      store_token_vote(p, votes, option_id);
   else
      store_vote(p, votes, option_id, 1);

   eosio::print("Vote stored!");
}

// Macro to register the contract's actions
EOSIO_ABI(pollgf, (newpoll)(newtokenpoll)(vote))