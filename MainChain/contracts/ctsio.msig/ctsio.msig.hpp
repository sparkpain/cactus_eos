#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
using namespace std;
namespace eosio {

   class msig : public contract {
      public:
         msig( account_name self ):contract(self){}
         void propose( uint64_t proposal_id, vector<permission_level> requested_permissions, 
                       vector<permission_level> trx_permissions, string contract_name, string action_name,
                       account_name from, account_name to );
         void approve( uint64_t proposal_id, permission_level level );
      private:
         //uint64_t proposal_id = 1;
         // @abi table ctsp
         struct ctsproposal {
            uint64_t                   proposal_id;
            account_name               from;
            //vector<char>               packed_transaction;
            //transaction_id_type        transaction_id;
            //EOSLIB_SERIALIZE( ctsproposal, (proposal_id)(from))
            auto primary_key()const { return proposal_id; }
            //account_name get_from()const { return from; }
         };
         typedef eosio::multi_index<N(ctsproposal),ctsproposal> ctsproposals;
         
         // @abi table ctsapproval
         struct ctsapproval {
            uint64_t                   proposal_id;
            vector<permission_level>   requested_approvals;
            vector<permission_level>   provided_approvals; 

            auto primary_key()const { return proposal_id; }
         };
         typedef eosio::multi_index<N(ctsapproval),ctsapproval> ctsapprovals;
   };

} /// namespace eosio
