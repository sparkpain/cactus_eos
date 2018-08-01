//
// Created by 超超 on 2018/8/1.
//

#include <eosio/sidechain_plugin/sidechain_plugin.hpp>

#include <eosio/chain/controller.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio/chain/action.hpp>

namespace eosio {
	using namespace chain;
//	using boost::signals2::

	static appbase::abstract_plugin &_sidechain_plugin = app().register_plugin<sidechain_plugin>();


	struct transaction_summary_object
			: public chainbase::object<transaction_symmary_object_type, transaction_summary_object> {
		OBJECT_CTOR(transaction_summary_object)


		id_type id;
		uint32_t block_num;
		transaction_id_type trx_id;

		account_name from;
		account_name to;
		asset quantity;
	};

	struct by_block;
	struct by_trx;
	using transaction_summary_multi_index = chainbase::shared_multi_index_container<
			transaction_summary_object,
			indexed_by<
					ordered_unique<tag<by_id>, member<transaction_summary_object, transaction_summary_object::id_type, &transaction_summary_object::id>>,
					ordered_unique<tag<by_trx>, member<transaction_summary_object, transaction_id_type, &transaction_summary_object::trx_id>>,
					ordered_non_unique<tag<by_block>, member<transaction_summary_object, uint32_t, &transaction_summary_object::block_num>>
			>
	>;

}

CHAINBASE_SET_INDEX_TYPE(eosio::transaction_summary_object, eosio::transaction_summary_multi_index)

namespace eosio { namespace chain {
	struct cactus {
		account_name  from;
		account_name  to;
		asset         quantity;

		cactus() = default;

		cactus(const account_name &from, const account_name &to, const asset &quantity) : from(from),to(to),
																						  quantity(quantity){}
		static name get_account() {
			return N(cactus);
		}

		static name get_name() {
			return N(transfer);
		}
	};
}} /// namespace eosio::chain

FC_REFLECT( eosio::chain::cactus, (from)(to)(quantity))

namespace eosio {

	class sidechain_plugin_impl {
		public:
			chain_plugin*       chain_plug = nullptr;
			void accepted_cactus_transfer(const transaction_metadata_ptr& trx) {
				auto& chain = chain_plug->chain();
				auto& db = chain.db();
				auto block_num = chain.pending_block_state()->block_num;
				for (const auto action : trx->trx.actions) {
					wlog("超: ${act}, ${name}", ("act", action.account)("name", action.name));
					if(action.account == chain::cactus::get_account() && action.name == chain::cactus::get_name()) {
						auto data = action.data_as<chain::cactus>();
						const auto transaction_summary = db.create<transaction_summary_object>([&](auto& a) {
							a.block_num = block_num;
							a.trx_id = trx->signed_id;
							a.from = data.from;
							a.to = data.to;
							a.quantity = data.quantity;
						});
						break;
					}
				}
			}

			void send_cactus_transfer(const block_state_ptr& irb) {
				auto& chain = chain_plug->chain();
				auto& db = chain.db();


	wlog("测试===================");
				while(1){
					;
				}
				const auto& tsmi = db.get_index<transaction_summary_multi_index, by_block>();
				vector<transaction_summary_object> irreversible_transactions;
				auto itr = tsmi.begin();
				while( itr != tsmi.end()) {
					if (itr->block_num <= irb->block_num) {
						ilog("tx-id: ${num}==${id},data【${from} -> ${to} ${quantity}】",
							 ("num",itr->block_num)("id",itr->trx_id)("from",itr->from)
									 ("to", itr->to)("quantity", itr->quantity));
						db.remove(*itr);
					}
					++ itr;
				}
	wlog("结束测试===================");
			}
	};

	sidechain_plugin::sidechain_plugin()
	:my(std::make_shared<sidechain_plugin_impl>()) {
	}

	sidechain_plugin::~sidechain_plugin() {
	}

	void sidechain_plugin::set_program_options(options_description& cli, options_description& cfg) {
		//nothing to do now
	}

	void sidechain_plugin::plugin_initialize(const variables_map& options) {
		try {
			//no option to deal now
			my->chain_plug = app().find_plugin<chain_plugin>();
			auto& chain = my->chain_plug->chain();

			chain.db().add_index<transaction_summary_multi_index>();

			chain.accepted_transaction.connect( [&](const transaction_metadata_ptr& trx) {
				my->accepted_cactus_transfer(trx);
			});
			chain.irreversible_block.connect( [&](const block_state_ptr& irb) {
				my->send_cactus_transfer(irb);
			});

		} FC_LOG_AND_RETHROW()
	}

	void sidechain_plugin::plugin_startup() {
	}

	void sidechain_plugin::plugin_shutdown() {
	}


}