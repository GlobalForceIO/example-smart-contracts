/**
 *  @file
 *  EOSIO Messenger Contract
 *  Enables sending, receiving, and deleting private messages on chain.
 */

#include <utility>
#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>        // EOSIO contract base class and macros
#include <eosiolib/asset.hpp>        // For asset types (not used in this contract)
#include <eosiolib/contract.hpp>     // EOSIO contract base
#include <eosiolib/time.hpp>         // EOSIO time and time_point_sec
#include <eosiolib/print.hpp>        // EOSIO print for debugging (not used in production)
#include <eosiolib/transaction.hpp>  // For inline/deferred transactions (not used here)

using namespace eosio;

/**
 * @class messenger
 * Implements a simple on-chain messenger with message sending, receiving, and deletion.
 */
class messenger : public eosio::contract
{
public:
  using contract::contract;

  // Contract constructor
  messenger(account_name self) : contract(self) {}

  /**
   * @brief Send a message from one account to another.
   * - Stores a notification and a message record on chain.
   * - The notification enables the recipient to find new incoming messages.
   * @param from  Sender account (must authorize)
   * @param to    Recipient account
   * @param msg   Message text (must not be empty)
   * @abi action
   */
  void sendmsg(const account_name from,
               const account_name to,
               const std::string msg)
  {
    require_auth(from);  // Ensure sender authorized

    eosio_assert(msg.size() > 0, "Empty message");

    // (Optional) Check that recipient account "to" exists

    notification_table notifications(_self, _self); // Notifications table (global scope)
    message_table messages(_self, from);            // Messages table (scoped to sender)

    uint64_t newid = notifications.available_primary_key(); // Unique message/notification id

    // Add a notification for the recipient (so they can find new messages)
    notifications.emplace(from, [&](auto &n) {
      n.id = newid;
      n.from = from;
      n.to = to;
    });

    // Store the actual message (including text and timestamp)
    messages.emplace(from, [&](auto &m) {
      m.id = newid;
      m.to = to;
      m.text = msg;
      m.send_at = eosio::time_point_sec(now());
      m.type = 0; // Reserved for future message type expansion
    });
  }

  /**
   * @brief Receive (and delete) a message that was sent to the recipient.
   * - The notification and message are deleted.
   * - Only the recipient can call this action.
   * @param to  The recipient account (must authorize)
   * @param id  The message/notification id
   * @abi action
   */
  void receivemsg(const account_name to, uint64_t id)
  {
    require_auth(to);

    notification_table notifications(_self, _self);
    auto itr_notif = notifications.find(id);
    eosio_assert(itr_notif != notifications.end(), "Notification not found");
    const auto &notif = *itr_notif;

    eosio_assert(notif.to == to, "Message not addressed to your account");

    message_table messages(_self, notif.from); // Message stored in sender's scope
    auto itr_msg = messages.find(id);
    eosio_assert(itr_msg != messages.end(), "Message not found");

    // Remove notification and message
    notifications.erase(itr_notif);
    messages.erase(itr_msg);
  }

  /**
   * @brief Delete a message sent by the sender (without recipient reading it).
   * - Only the sender can call this action.
   * - The notification and the message are deleted.
   * @param from  Sender account (must authorize)
   * @param id    The message/notification id
   * @abi action
   */
  void erasemsg(const account_name from, uint64_t id)
  {
    require_auth(from);

    notification_table notifications(_self, _self);
    auto itr_notif = notifications.find(id);
    eosio_assert(itr_notif != notifications.end(), "Notification not found");
    const auto &notif = *itr_notif;

    eosio_assert(notif.from == from, "Message was not sent from your account");

    message_table messages(_self, from); // Message stored in sender's scope
    auto itr_msg = messages.find(id);
    eosio_assert(itr_msg != messages.end(), "Message not found");

    // Remove notification and message
    notifications.erase(itr_notif);
    messages.erase(itr_msg);
  }

private:

  /**
   * @struct message
   * @brief Table structure for messages sent from this user.
   * - Scoped by sender's account.
   * - Stores recipient, message text, send time, and type.
   * @abi table message i64
   */
  struct message
  {
    uint64_t id;                   // Unique message id
    account_name to;               // Recipient account
    std::string text;              // Message body
    eosio::time_point_sec send_at; // Timestamp of when sent
    uint8_t type;                  // Reserved for future message types

    uint64_t primary_key() const { return id; }

    EOSLIB_SERIALIZE(message, (id)(to)(text)(send_at)(type))
  };
  typedef eosio::multi_index<N(message), message> message_table;

  /**
   * @struct notification
   * @brief Table structure for notifications of new messages.
   * - Stored globally (scope: contract).
   * - Each notification has sender and recipient accounts.
   * @abi table notification i64
   */
  struct notification
  {
    uint64_t id;           // Unique notification id (matches message id)
    account_name from;     // Sender account
    account_name to;       // Recipient account

    uint64_t primary_key() const { return id; }
     account_name get_to_key() const { return to; }
    
    EOSLIB_SERIALIZE(notification, (id)(from)(to))
  };

  typedef eosio::multi_index<N(notification), notification,
                             eosio::indexed_by<N(to),
                                               eosio::const_mem_fun<
                                                   notification,
                                                   account_name,
                                                   &notification::get_to_key>>>
      notification_table;

};

EOSIO_ABI(messenger, (sendmsg)(receivemsg)(erasemsg))