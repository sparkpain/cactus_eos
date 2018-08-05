/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/exceptions.hpp>
#include <iostream>
#include <regex>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/process/spawn.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <eosio/client_plugin/client_plugin.hpp>
#include <eosio/client_plugin/client_manager.hpp>


namespace eosio {
    using namespace client::http;
    static appbase::abstract_plugin& _client_plugin = app().register_plugin<client_plugin>();

    string chain_url= "http://127.0.0.1:8888/";
    string wallet_url= "http://127.0.0.1:8900/";

    bool no_verify = false;
    auto   tx_expiration = fc::seconds(30);
    string tx_ref_block_num_or_id;
    bool   tx_force_unique = false;
    uint8_t  tx_max_cpu_usage = 0;
    uint32_t tx_max_net_usage = 0;

    template<typename T>
    T dejsonify(const string& s) {
       return fc::json::from_string(s).as<T>();
    }


chain::private_key_type client_prikey;
    client_plugin::client_plugin(){}
    client_plugin::~client_plugin(){}

    void client_plugin::set_program_options(options_description&, options_description& cfg) {
        cfg.add_options()
              ("client-wallet-address", bpo::value<string>()->default_value("http://127.0.0.1:8900/"),
              "The remote BP's IP and port to listen for incoming http connections; set blank to disable.")
              ("client-chain-address", bpo::value<string>()->default_value("http://127.0.0.1:8888/"),
              "The remote BP's IP and port to listen for incoming http connections; set blank to disable.")
              ("client-private-key", bpo::value<string>()->default_value("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"),
               "client plugin's private key")
          ;
    }


    void client_plugin::plugin_initialize(const variables_map& options){
        try {
            if( options.count( "client-wallet-address" ) && options.at("client-wallet-address").as<string>().length
            ()) {
                string lipstr_wallet = options.at( "client-wallet-address" ).as<string>();

                wallet_url.assign(lipstr_wallet);
                ilog( "client_plugin configured wallet http to ${h}", ("h", wallet_url));
            }

            if( options.count( "client-chain-address" ) && options.at("client-chain-address").as<string>().length
            ()) {
               string lipstr_chain = options.at( "client-chain-address" ).as<string>();

               chain_url.assign(lipstr_chain);
               ilog( "client_plugin configured chain http to ${h}", ("h", chain_url));
            }

            if( options.count("client-private-key") )
             {

                client_prikey = private_key_type(options.at( "client-private-key" ).as<string>());
                ilog("Keosd use private key: ${priv}",("priv", client_prikey));
             }

        }FC_LOG_AND_RETHROW()

    }

    void client_plugin::plugin_startup(){
        ilog("initializing client plugin");

      // auto client_apis = app().get_plugin<client_plugin>().get_client_apis();
      // auto info = client_apis.get_info("http://127.0.0.1:8888");
      //   std::cout << fc::json::to_pretty_string(info) << std::endl;
      // string chain_url = "http://127.0.0.1:8888";
         // auto transaction_info = client_apis.get_transaction(chain_url,10022,"3331399d4c1d5ebe8aa67c9e429b9137417794448b5060c2112ae5624bd8c7ba");
         // std::cout << fc::json::to_pretty_string(transaction_info) << std::endl;

         // std::vector<string> v;
         // v.push_back("eosio");
         // client_apis.push_action(chain_url,"cactus","transfer","[\"eosio\",\"cactus\",\"100.0000 SYS\", \"m\"]",v);


         // /cleos push action cactus transfer '["eosio","cactus","25.0000 SYS", "m"]' -p eosio

          // auto public_keys = call(wallet_url, wallet_public_keys, fc::variant());
          //   wlog("result:${result}",("result",public_keys));

          // std::vector<string> permission;
          // permission.push_back("eosio.msig");
          // multisig_propose("sidechain", "[{\"actor\":\"shengfeng\",\"permission\":\"active\"},{\"actor\":\"haonan\",\"permission\":\"active\"}]", "[{\"actor\":\"yuanchao\",\"permission\":\"active\"}]","eosio.token","transfer","{\"from\":\"yuanchao\",\"to\":\"haonan\",\"quantity\":\"40.0000 SYS\",\"memo\":\"test multisig\"}",permission);

    }

    void client_plugin::plugin_shutdown(){}

namespace client_apis{

     client_cactus::client_cactus(){

    }

    template<typename T>
    fc::variant client_cactus::call( const std::string& url,
                      const std::string& path,
                      const T& v ) {
        auto urlpath = parse_url(url) + path;
        http_context context = create_http_context();
        connection_param *cp = new connection_param(context, urlpath, false);

        return do_http_call( *cp, fc::variant(v), true, true );

    }

    eosio::chain_apis::read_only::get_info_results client_cactus::get_info( const std::string& url) {
        auto info = call(url, get_info_func, fc::variant()).as<eosio::chain_apis::read_only::get_info_results>();
        // std::cout << fc::json::to_pretty_string(transaction) << std::endl;
        return info;
    }

    fc::variant client_cactus::get_transaction( const std::string& url, uint32_t block_num_hint,
                                             string transaction_id_str) {
        transaction_id_type transaction_id;
        try {
            while( transaction_id_str.size() < 64 ) transaction_id_str += "0";
            transaction_id = transaction_id_type(transaction_id_str);
        } EOS_RETHROW_EXCEPTIONS(transaction_id_type_exception, "Invalid transaction ID: ${transaction_id}", ("transaction_id", transaction_id_str))
        auto arg= fc::mutable_variant_object( "id", transaction_id);
        if ( block_num_hint > 0 ) {
            arg = arg("block_num_hint", block_num_hint);
        }
        auto transaction_info =call(url,get_transaction_func,fc::variant(arg));

        // std::cout << fc::json::to_pretty_string(transaction_info) << std::endl;
        return transaction_info;
    }

    void client_cactus::push_action(const std::string& url,string contract_account,string action,string data,
                                    const vector<string>& tx_permission ){
          fc::variant action_args_var;
          if( !data.empty() ) {
              try {
                  action_args_var = json_from_file_or_string(data, fc::json::relaxed_parser);
              } EOS_RETHROW_EXCEPTIONS(action_type_exception, "Fail to parse action JSON data='${data}'", ("data", data))
          }

          auto arg= fc::mutable_variant_object
                    ("code", contract_account)
                    ("action", action)
                    ("args", action_args_var);
          auto result = call(url, json_to_bin_func, arg);
          wlog("result:${result}",("result",result));
          auto accountPermissions = get_account_permissions(tx_permission); //vector<string> tx_permission;

          send_actions({eosio::chain::action{accountPermissions, contract_account, action, result.get_object()["binargs"].as<bytes>()}});
    }


    void client_cactus::send_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu, packed_transaction::compression_type compression) {
          auto result = push_actions( move(actions), extra_kcpu, compression);
          std::cout << fc::json::to_pretty_string( result ) << std::endl;
    }

    fc::variant client_cactus::push_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu, packed_transaction::compression_type compression) {
        signed_transaction trx;
        trx.actions = std::forward<decltype(actions)>(actions);
        return push_transaction(trx, extra_kcpu, compression);
    }



    fc::variant client_cactus::push_transaction( signed_transaction& trx, int32_t extra_kcpu, packed_transaction::compression_type compression) {
        auto info = get_info(chain_url);
        trx.expiration = info.head_block_time + tx_expiration;

        // Set tapos, default to last irreversible block if it's not specified by the user
        block_id_type ref_block_id = info.last_irreversible_block_id;
        // try {
        //    fc::variant ref_block;
        //    if (!tx_ref_block_num_or_id.empty()) {
        //       ref_block = call(get_block_func, fc::mutable_variant_object("block_num_or_id", tx_ref_block_num_or_id));
        //       ref_block_id = ref_block["id"].as<block_id_type>();
        //    }
        // } EOS_RETHROW_EXCEPTIONS(invalid_ref_block_exception, "Invalid reference block num or id:  ${block_num_or_id}", ("block_num_or_id", tx_ref_block_num_or_id));
        trx.set_reference_block(ref_block_id);

        // if (tx_force_unique) {
        //    trx.context_free_actions.emplace_back( generate_nonce_action() );
        // }

        trx.max_cpu_usage_ms = tx_max_net_usage;
        trx.max_net_usage_words = (tx_max_net_usage + 7)/8;

        sign_transaction_local(trx, client_prikey, info.chain_id);
        return call(chain_url, push_txn_func, packed_transaction(trx, compression));
    }

    void client_cactus::sign_transaction_local(signed_transaction& trx,  const private_key_type& private_key,
                                               const chain_id_type& chain_id) {
         optional<signature_type> sig = private_key.sign(trx.sig_digest(chain_id, trx.context_free_data));
         if (sig) {
            trx.signatures.push_back(*sig);
         }
    }

    fc::variant client_cactus::json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype = fc::json::legacy_parser)
    {
        std::regex r("^[ \t]*[\{\[]");
       if ( !regex_search(file_or_str, r) && fc::is_regular_file(file_or_str) ) {
          return fc::json::from_file(file_or_str, ptype);
       } else {
          return fc::json::from_string(file_or_str, ptype);
       }
    }

    vector<chain::permission_level> client_cactus::get_account_permissions(const vector<string>& permissions) {
        auto fixedPermissions = permissions | boost::adaptors::transformed([](const string& p) {
            vector<string> pieces;
            split(pieces, p, boost::algorithm::is_any_of("@"));
            if( pieces.size() == 1 ) pieces.push_back( "active" );
                return chain::permission_level{ .actor = pieces[0], .permission = pieces[1] };
        });
        vector<chain::permission_level> accountPermissions;
        boost::range::copy(fixedPermissions, back_inserter(accountPermissions));
        return accountPermissions;
    }

    fc::variant client_cactus::determine_required_keys(const signed_transaction& trx) {
        // TODO better error checking
        //wdump((trx));
        const auto& public_keys = call(wallet_url, wallet_public_keys, fc::variant());
        auto get_arg = fc::mutable_variant_object
               ("transaction", (transaction)trx)
               ("available_keys", public_keys);

        const auto& required_keys = call(chain_url, get_required_keys, get_arg);
        wlog("required_keys:${result}",("result",required_keys));
        return required_keys["required_keys"];
    }

    void client_cactus::sign_transaction(signed_transaction& trx, fc::variant& required_keys, const chain_id_type& chain_id) {
        fc::variants sign_args = {fc::variant(trx), required_keys, fc::variant(chain_id)};
        const auto& signed_trx = call(wallet_url, wallet_sign_trx, sign_args);
        trx = signed_trx.as<signed_transaction>();
        wlog("signed_trx:${result}",("result",trx));
    }





    unsigned int proposal_expiration_hours = 24;
    void client_cactus::multisig_propose(string proposal_name, string requested_perm, string transaction_perm, string proposed_contract, string proposed_action, string proposed_transaction,const vector<string>& tx_permission){//string proposer,
          fc::variant requested_perm_var;
          try {
             requested_perm_var = json_from_file_or_string(requested_perm);
          } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse permissions JSON '${data}'", ("data",requested_perm))
          fc::variant transaction_perm_var;
          try {
             transaction_perm_var = json_from_file_or_string(transaction_perm);
          } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse permissions JSON '${data}'", ("data",transaction_perm))
          fc::variant trx_var;
          try {
             trx_var = json_from_file_or_string(proposed_transaction);
          } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse transaction JSON '${data}'", ("data",proposed_transaction))
          transaction proposed_trx = trx_var.as<transaction>();

          auto arg = fc::mutable_variant_object()
             ("code", proposed_contract)
             ("action", proposed_action)
             ("args", trx_var);

          auto result = call(chain_url, json_to_bin_func, arg);

          bytes proposed_trx_serialized = result.get_object()["binargs"].as<bytes>();

          vector<permission_level> reqperm;
          try {
             reqperm = requested_perm_var.as<vector<permission_level>>();
          } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Wrong requested permissions format: '${data}'", ("data",requested_perm_var));

          vector<permission_level> trxperm;
          try {
             trxperm = transaction_perm_var.as<vector<permission_level>>();
          } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Wrong transaction permissions format: '${data}'", ("data",transaction_perm_var));

          auto accountPermissions = get_account_permissions(tx_permission);
          // if (accountPermissions.empty()) {
          //    if (!proposer.empty()) {
          //       accountPermissions = vector<permission_level>{{proposer, config::active_name}};
          //    } else {
          //       EOS_THROW(missing_auth_exception, "Authority is not provided (either by multisig parameter <proposer> or -p)");
          //    }
          // }
          // if (proposer.empty()) {
            auto proposer = chain::name(accountPermissions.at(0).actor).to_string();
          // }

          transaction trx;

          trx.expiration = fc::time_point_sec( fc::time_point::now() + fc::hours(proposal_expiration_hours) );
          trx.ref_block_num = 0;
          trx.ref_block_prefix = 0;
          trx.max_net_usage_words = 0;
          trx.max_cpu_usage_ms = 0;
          trx.delay_sec = 0;
          trx.actions = { chain::action(trxperm, chain::name(proposed_contract), chain::name(proposed_action), proposed_trx_serialized) };

          fc::to_variant(trx, trx_var);

          arg = fc::mutable_variant_object()
             ("code", "eosio.msig")
             ("action", "propose")
             ("args", fc::mutable_variant_object()
              ("proposer", proposer )
              ("proposal_name", proposal_name)
              ("requested", requested_perm_var)
              ("trx", trx_var)
             );
          result = call(chain_url, json_to_bin_func, arg);

          send_actions({chain::action{accountPermissions, "eosio.msig", "propose", result.get_object()["binargs"].as<bytes>()}});

    }


}

}

