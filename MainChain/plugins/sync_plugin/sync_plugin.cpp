//
// Created by 超超 on 2018/8/1.
//

#include <eosio/sync_plugin/sync_plugin.hpp>

#include <eosio/chain/controller.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/client_plugin/client_plugin.hpp>

#include <eosio/chain/action.hpp>

namespace eosio { namespace chain {
	struct cactus_transfer {
		account_name from;
		account_name to;
		asset quantity;

		cactus_transfer() = default;
		cactus_transfer(const account_name &from, const account_name &to, const asset &quantity) : from(from), to(to), quantity(quantity) {}
	};
}}
FC_REFLECT( eosio::chain::cactus_transfer, (from)(to)(quantity))

#ifndef DEFINE_INDEX
#define DEFINE_INDEX(object_type, object_name, index_name) \
	struct object_name \
			: public chainbase::object<object_type, object_name> { \
			OBJECT_CTOR(object_name) \
			id_type id; \
			uint32_t block_num; \
			transaction_id_type trx_id; \
			action_data data; \
	}; \
	\
	struct by_block_num; \
	struct by_trx_id; \
	using index_name = chainbase::shared_multi_index_container< \
		object_name, \
		indexed_by< \
				ordered_unique<tag<by_id>, member<object_name, object_name::id_type, &object_name::id>>, \
				ordered_unique<tag<by_trx_id>, member<object_name, transaction_id_type, &object_name::trx_id>>, \
				ordered_non_unique<tag<by_block_num>, member<object_name, uint32_t, &object_name::block_num>> \
		> \
	>;
#endif

#ifndef DATA_FORMAT
#define DATA_FORMAT(user, trx_id, to, quantity) "[\""+user+"\", \""+trx_id+"\", \""+to+"\", \""+quantity+"\"]"
#endif

namespace eosio {

	using namespace chain;
	using action_data = vector<char>;

	static appbase::abstract_plugin &_sync_plugin = app().register_plugin<sync_plugin>();

	DEFINE_INDEX(transaction_reversible_object_type, transaction_reversible_object, transaction_reversible_multi_index)
	DEFINE_INDEX(transaction_executed_object_type, transaction_executed_object, transaction_executed_multi_index)
}

CHAINBASE_SET_INDEX_TYPE(eosio::transaction_reversible_object, eosio::transaction_reversible_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::transaction_executed_object, eosio::transaction_executed_multi_index)

namespace eosio {

	class sync_plugin_impl {
		public:
			chain_plugin*       chain_plug = nullptr;
			fc::microseconds 	_max_irreversible_transaction_age_us;
			bool				_send_propose_enabled = false;
			string 				_peer_chain_address;
			string 				_peer_chain_account;
			string 				_peer_chain_constract;
			string 				_my_chain_constract;


			optional<boost::signals2::scoped_connection> accepted_transaction_connection;
			optional<boost::signals2::scoped_connection> irreversible_block_connection;

			void accepted_transaction(const transaction_metadata_ptr& trx) {
				auto& chain = chain_plug->chain();
				auto& db = chain.db();

				auto block_num = chain.pending_block_state()->block_num;
				auto now = fc::time_point::now();
				auto block_age = (chain.pending_block_time() > now) ? fc::microseconds(0) : (now - chain.pending_block_time());

				if (!_send_propose_enabled) {
					return;
				} else if ( _max_irreversible_transaction_age_us.count() >= 0 && block_age >= _max_irreversible_transaction_age_us ) {
					return;
				}

				for (const auto action : trx->trx.actions) {
					if (action.account == name(_my_chain_constract)
						&& action.name == N(transfer)) {
						const auto* existed = db.find<transaction_reversible_object, by_trx_id>(trx->id);
						if (existed != nullptr) {
							return;
						}

						const auto transaction_reversible = db.create<transaction_reversible_object>([&](auto& tso) {
							tso.block_num = block_num;
							tso.trx_id = trx->id;
							tso.data = action.data;
						});
						break;
					}
				}
			}

			void send_transaction(const block_state_ptr& irb) {
				auto& chain = chain_plug->chain();
				auto& db = chain.db();

				const auto& tsmi = db.get_index<transaction_reversible_multi_index, by_block_num>();
				auto itr = tsmi.begin();
				while( itr != tsmi.end()) {
					if (itr->block_num <= irb->block_num) {
						auto data = fc::raw::unpack<cactus_transfer>(itr->data);
						ilog("tx-id: ${num}==${id},data【${from} -> ${to} ${quantity}】",
							 ("num",itr->block_num)("id",itr->trx_id)("from",data.from)
									 ("to", data.to)("quantity", data.quantity));

						// need to validate ?

						// send propose or approve
						string datastr = DATA_FORMAT(_peer_chain_account, string(itr->trx_id), string(data.to), data.quantity.to_string());
						vector<string> permissions = {_peer_chain_account};
						app().find_plugin<client_plugin>()->get_client_apis().push_action(_peer_chain_address, _peer_chain_constract, "msigtrans", datastr, permissions);

						// remove or move to other table ?
						db.remove(*itr);
					}
					++ itr;
				}
			}
	};

	sync_plugin::sync_plugin()
	:my(std::make_shared<sync_plugin_impl>()) {
	}

	sync_plugin::~sync_plugin() {
	}

	void sync_plugin::set_program_options(options_description& cli, options_description& cfg) {
		cfg.add_options()
				("max-irreversible-transaction-age", bpo::value<int32_t>()->default_value( -1 ), "Max irreversible age of transaction to deal")
				("enable-send-propose", bpo::bool_switch()->notifier([this](bool e){my->_send_propose_enabled = e;}), "Enable push propose.")
				("peer-chain-address", bpo::value<string>()->default_value("http://127.0.0.1:8899/"), "In MainChain it is SideChain address, otherwise it's MainChain address")
				("peer-chain-account", bpo::value<string>()->default_value("cactus"), "In MainChain it is your SideChain's account, otherwise it's your MainChain's account")
				("peer-chain-constract", bpo::value<string>()->default_value("cactus"), "In MainChain it is SideChain's cactus constract, otherwise it's MainChain's cactus constract")
				("my-chain-constract", bpo::value<string>()->default_value("cactus"), "this chain's cactus contract")
			;
	}

	void sync_plugin::plugin_initialize(const variables_map& options) {
		try {
			my->_max_irreversible_transaction_age_us = fc::seconds(options.at("max-irreversible-transaction-age").as<int32_t>());
			my->_peer_chain_address = options.at("peer-chain-address").as<string>();
			my->_peer_chain_account = options.at("peer-chain-account").as<string>();
			my->_peer_chain_constract = options.at("peer-chain-constract").as<string>();
			my->_my_chain_constract = options.at("my-chain-constract").as<string>();

			my->chain_plug = app().find_plugin<chain_plugin>();
			auto& chain = my->chain_plug->chain();

			chain.db().add_index<transaction_reversible_multi_index>();
			chain.db().add_index<transaction_executed_multi_index>();

			my->accepted_transaction_connection.emplace(chain.accepted_transaction.connect( [&](const transaction_metadata_ptr& trx) {
				my->accepted_transaction(trx);
			}));
			my->irreversible_block_connection.emplace(chain.irreversible_block.connect( [&](const block_state_ptr& irb) {
				my->send_transaction(irb);
			}));

		} FC_LOG_AND_RETHROW()
	}

	void sync_plugin::plugin_startup() {
	}

	void sync_plugin::plugin_shutdown() {
		my->accepted_transaction_connection.reset();
		my->irreversible_block_connection.reset();
	}


}