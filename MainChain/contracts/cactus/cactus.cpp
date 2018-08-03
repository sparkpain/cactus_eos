#include <string> 


#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/crypto.h>

using namespace eosio;
using eosio::action;

class cactus : public eosio::contract {
  public:
      using contract::contract;

      cactus(account_name self):eosio::contract(self),
      cts(_self, _self){}
      //@abi action 
      void transfer(account_name from,asset quantity) {
         print( "Hello, ", name{from} );
         auto quant_after_fee = quantity;
         
         eosio_assert( is_account( from ), "to account does not exist");


         eosio_assert( quantity.is_valid(), "invalid quantity" );
         eosio_assert( quantity.amount > 0, "must withdraw positive quantity" );
         printf("%s\n", "开始校验from的权限");

         require_auth(_self);
         printf("%s\n", "开始转from的账");
         action(
              permission_level{ from, N(active) },
              N(eosio.token), N(transfer),
              std::make_tuple(from,_self, quantity, std::string("cactus transfer"))
            ).send();

         /*INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {payer,N(active)},
         { payer, N(eosio.ram), quant_after_fee, std::string("buy ram") } );*/

        //cts_his_index cts(_self, _self);






         /* uint64_t tmp_id =0;
         if(itr!=cts.end()){
            tmp_id = itr->id+1;
         }*/

         cts.emplace(_self, [&](auto& a){
                  a.id = cts.available_primary_key();;
                  a.from      = from;
                  a.amount = quantity.amount;
                  // a.creation_date = eosio::time_point_sec();
                  a.creation_date = eosio::time_point();
               });

         //printf("%s\n", "执行cactus transfer 结束，请确认到账情况");
      }

      //@abi action 
      void tmp(account_name from,account_name to,asset quantity) {
         print( "Hello, ", name{from} );
         
         //printf("%s\n", "开始转from的账");
         action(
              permission_level{ from, N(active) },
              N(eosio.token), N(transfer),
              std::make_tuple(from,to, quantity, std::string("cactus transfer"))
            ).send();

        

         printf("%s\n", "执行cactus tmp 结束，请确认到账情况");
      }

    private:
      //@abi table cactushi i64
      struct cactushi
      {

        uint64_t id;
        account_name from;
        int64_t amount;
        eosio::time_point_sec creation_date;
        uint64_t primary_key()const { return id; }

        EOSLIB_SERIALIZE( cactushi, (id)(from)(amount)(creation_date) )
      };

      typedef eosio::multi_index< N(cactushi), cactushi> cts_his_index;
      cts_his_index        cts;

};
EOSIO_ABI( cactus, (transfer)(tmp))
