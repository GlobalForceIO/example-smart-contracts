#include "database.hpp"

void testda::create(name user, string title, string content)
{
    require_auth(user);

    check(title.size() > 0, "Title cannot be empty");
    check(content.size() > 0, "Content cannot be empty");
    check(title.size() <= 128, "Title too long");
    check(content.size() <= 4096, "Content too long");

    das datable(get_self(), get_self().value); // One global table (can use user.value for user-scoped)

    datable.emplace(user, [&](auto& d) {
        d.post_id = datable.available_primary_key();
        d.poster = user;
        d.title = title;
        d.content = content;
    });
}

void testda::erase(name user, uint64_t post_id)
{
    das datable(get_self(), get_self().value);
    auto itr = datable.find(post_id);
    check(itr != datable.end(), "Post not found");
    check(itr->poster == user, "Only the poster can delete their post");
    require_auth(user);
    datable.erase(itr);
}