#pragma once
#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>
namespace eosio {
	using boost::signals2::signal;

	class sidechain_plugin : public appbase::plugin<sidechain_plugin> {
	public:
		APPBASE_PLUGIN_REQUIRES((chain_plugin))

		sidechain_plugin();

		virtual ~sidechain_plugin();

		virtual void set_program_options(options_description &cli, options_description &cfg) override;

		void plugin_initialize(const variables_map& options);
		void plugin_startup();
		void plugin_shutdown();

//		signal<void(const chain::producer_confirmation&)> confirmed_block;
	private:
		std::shared_ptr<class sidechain_plugin_impl> my;
	};
} /// namespace eosio