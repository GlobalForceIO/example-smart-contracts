#pragma once
#include <eosio/eosio.hpp>
#include <string>
using namespace eosio;
using std::string;

// Explicit contract name for abigen/--contract
class [[eosio::contract("database")]] database : public contract {
public:
  using contract::contract;

  // Create a new post
  [[eosio::action]]
  void create(name user, string title, string content);

  // Delete a post by id (only the author can delete)
  [[eosio::action]]
  void erase(name user, uint64_t post_id);

  // Posts table
  struct [[eosio::table]] da {
    uint64_t post_id;   // Auto-increment ID
    name     poster;    // Author
    string   title;
    string   content;

    uint64_t primary_key() const { return post_id; }
    uint64_t byposter()    const { return poster.value; }
  };

  // Index by author for querying: get_index<"byposter"_n>()
  using das = multi_index<
    "data"_n, da,
    indexed_by<"byposter"_n, const_mem_fun<da, uint64_t, &da::byposter>>
  >;
};
