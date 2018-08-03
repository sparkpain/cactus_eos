#include <ctsio.msig.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/permission.hpp>

namespace eosio {

  void msig::propose(uint64_t proposal_id, vector<permission_level> requested_permissions, 
                     vector<permission_level> trx_permissions, string contract_name, string action_name,
                     account_name from, account_name to) { 
     print(proposal_id);
     ctsproposals proptable( _self, _self );
     proptable.emplace( _self, [&]( auto& prop ) {
        prop.proposal_id         = proposal_id;
        prop.from                = from;
     }); 

     ctsapprovals apptable( _self, _self );
     apptable.emplace( _self, [&]( auto& appr) {
        appr.proposal_id            = proposal_id;
        appr.requested_approvals    = std::move(requested_permissions);
     });
  }

  void msig::approve( uint64_t proposal_id, permission_level level ){
      require_auth ( level );

      ctsapprovals apptable( _self, _self );
      auto& appr = apptable.get( proposal_id, "proposal not found");

      auto itr = std::find( appr.requested_approvals.begin(), appr.requested_approvals.end(), level );
      eosio_assert( itr != appr.requested_approvals.end(), "approval is not on the list of requested approvals" );

      apptable.modify( appr, _self, [&]( auto& a ) {
        a.provided_approvals.push_back( level );
        a.requested_approvals.erase( itr );
      });
  }


} /// namespace eosio

EOSIO_ABI( eosio::msig, (propose)(approve))
