#include <eosio/eosio.hpp>
using namespace eosio;

CONTRACT mycontract : public contract {
  public:
    using contract::contract;

    TABLE StoredData {
      uint64_t id;
      std::string text;
      
      uint64_t primary_key() const { return id; }
    };
    
    using storage_table = multi_index<"mytable"_n, StoredData>;

    ACTION save( uint64_t id, std::string text ) {
      storage_table _storage( get_self(), get_self().value );
      _storage.emplace( get_self(), [&]( auto& row ) {
        row.id = id;
        row.text = text;
      });
    }
};
