#pragma once

#include <eosio/eosio.hpp>
#include <string>

using namespace eosio;
using std::string;

CONTRACT testda : public contract {
public:
    using contract::contract;

    // Create new post
    [[eosio::action]]
    void create(name user, string title, string content);

    // (Optional) Remove post by id (can only be deleted by poster)
    [[eosio::action]]
    void erase(name user, uint64_t post_id);

    // Table for posts
    struct [[eosio::table]] da {
        uint64_t     post_id;     // Unique post id (auto increment)
        name         poster;      // Account who posted
        string       title;
        string       content;

        uint64_t primary_key() const { return post_id; }
        uint64_t byposter() const { return poster.value; }
    };

    using das = multi_index<
        "data"_n, da,
        indexed_by<"byposter"_n, const_mem_fun<da, uint64_t, &da::byposter>>
    >;
};