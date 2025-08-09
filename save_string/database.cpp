#include "database.hpp"

void database::create(name user, string title, string content) {
  require_auth(user);
  check(!title.empty(),   "Title cannot be empty");
  check(!content.empty(), "Content cannot be empty");
  check(title.size()   <= 128,  "Title too long");
  check(content.size() <= 4096, "Content too long");

  das datable(get_self(), get_self().value);
  datable.emplace(user, [&](auto& d){
    d.post_id = datable.available_primary_key();
    d.poster  = user;
    d.title   = title;
    d.content = content;
  });
}

void database::erase(name user, uint64_t post_id) {
  require_auth(user);
  das datable(get_self(), get_self().value);
  auto it = datable.find(post_id);
  check(it != datable.end(), "Post not found");
  check(it->poster == user, "Only the poster can delete their post");
  datable.erase(it);
}

// Keep dispatcher in the same TU
EOSIO_DISPATCH(database, (create)(erase))
