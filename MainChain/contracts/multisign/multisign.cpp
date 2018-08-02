/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

#include <string>
#include <vector>
#include <set>
using std::string;
using std::vector;
using std::set;
using namespace eosio;

class multisign : public eosio::contract {
  public:
      using contract::contract;


      multisign(account_name self)
      :eosio::contract(self)
      ,multi_trxs(_self, _self)
      {}


      /// @abi action 
      void hi( account_name user, checksum256& trx_id, account_name to, asset quantity) {
          eosio_assert( account_set.count(user) > 0, "user is not witness" );
          require_auth(user);

          auto idx = multi_trxs.template get_index<N(trx_id)>();
          auto curr_multi_trx = idx.find( multi_trx::get_trx_id(trx_id)); 

          if (curr_multi_trx == idx.end()) {
              multi_trxs.emplace( _self, [&]( auto& a ){
                 a.id = multi_trxs.available_primary_key();
                 a.trx_id = trx_id;
                 a.to = to;
                 a.quantity = quantity;
                 a.confirmed.push_back(user);
             });
          } else {
              eosio_assert( curr_multi_trx->to == to, "to account is not correct" );
              eosio_assert( curr_multi_trx->quantity == quantity, "quantity is not correct" );

              idx.modify(curr_multi_trx, 0, [&]( auto& a ) {
                    eosio_assert( std::find(a.confirmed.begin(),a.confirmed.end(),user) == a.confirmed.end(), 
                            "this account already confirmed" );     
                    a.confirmed.push_back(user);
              });

              if (curr_multi_trx->confirmed.size() == required_confs) {
                  action(
                      permission_level{ _self, N(active) },
                      N(eosio.token), N(transfer),
                      std::make_tuple(_self, to, quantity, std::string("cactus transfer"))
                    ).send();
              }
          }          
      }

  private:
    //@abi table multi_trx i64
    struct multi_trx {
        uint64_t              id;
        checksum256           trx_id;
        account_name          to;
        asset                 quantity;
        vector<account_name>  confirmed;

        uint64_t primary_key() const { return id; }

        key256 by_trx_id()const { return get_trx_id(trx_id); }

         static key256 get_trx_id(const checksum256& trx_id) {
            const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&trx_id);
            return key256::make_from_word_sequence<uint64_t>(p64[0], p64[1], p64[2], p64[3]);
         }

        EOSLIB_SERIALIZE( multi_trx, (id)(trx_id)(to)(quantity)(confirmed) )
    };

    typedef eosio::multi_index< N(multi_trx), multi_trx, 
      indexed_by< N(trx_id), const_mem_fun<multi_trx, key256, &multi_trx::by_trx_id> >
    > multi_trx_index;

    multi_trx_index multi_trxs;

    set<account_name> account_set = { N(hello), N(eosio.token), N(eosio),
                                      N(sf), N(haonan), N(zhd), N(longge) };
    uint32_t required_confs = (uint32_t)(account_set.size() * 2 / 3) + 1;
};

EOSIO_ABI( multisign, (hi) )
