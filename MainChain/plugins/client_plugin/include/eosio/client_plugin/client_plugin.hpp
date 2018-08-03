/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <iostream>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_client_plugin/http_client_plugin.hpp>
#include <fc/io/json.hpp>

namespace eosio {

using namespace appbase;

 namespace client {
  namespace http {
      class client_manager;
   }
 }

using namespace client::http;
using namespace eosio::chain;
using boost::signals2::signal;
class client_plugin : public appbase::plugin<client_plugin> {
public:
   client_plugin();
   virtual ~client_plugin();

   APPBASE_PLUGIN_REQUIRES((chain_plugin))
   virtual void set_program_options(options_description&, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   // history_apis::read_only  get_client_api()const { return history_apis::read_only(history_const_ptr(my)); }

   client_manager& get_client_manager();
   fc::variant get_transaction( const std::string& url, uint32_t block_num_hint,
                                         string transaction_id_str);
   eosio::chain_apis::read_only::get_info_results  get_info( const std::string& url);
   void push_action(const std::string& url,string contract,string action,string data,
                     const vector<string>& tx_permission );
   void send_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none);
   fc::variant push_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none);
   fc::variant push_transaction( signed_transaction& trx, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none);
   void multisig_propose(string proposal_name, string requested_perm, string transaction_perm, string proposed_contract, string proposed_action, string proposed_transaction,const vector<string>& tx_permission);

private:
   template<typename T>
   fc::variant call( const std::string& url,
                  const std::string& path,
                  const T& v );
   fc::variant json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype);
   vector<chain::permission_level> get_account_permissions(const vector<string>& permissions);
   fc::variant determine_required_keys(const signed_transaction& trx);
   void sign_transaction(signed_transaction& trx, fc::variant& required_keys, const chain_id_type& chain_id);

   std::unique_ptr<client_manager> client_manager_ptr;
   std::shared_ptr<class client_plugin_impl> my;
};

}