/*
 * A test of handling an exception during binding to dispatcher.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>

#include <ace/OS.h>
#include <ace/Time_Value.h>

#include <so_5/h/types.hpp>
#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>
#include <so_5/disp/active_obj/h/pub.hpp>

so_5::atomic_counter_t g_agents_count = 0;
so_5::atomic_counter_t g_evt_count = 0;

so_5::rt::nonempty_name_t g_test_mbox_name( "test_mbox" );

struct some_message
	:
		public so_5::rt::message_t
{
	some_message() {}
	virtual ~some_message(){}
};

class a_ordinary_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:
		a_ordinary_t(
			so_5::rt::so_environment_t & env )
			:
				base_type_t( env )
		{
			++g_agents_count;
		}

		virtual ~a_ordinary_t()
		{
			--g_agents_count;
		}

		virtual void
		so_define_agent()
		{
			so_5::rt::mbox_ref_t mbox = so_environment()
				.create_local_mbox( g_test_mbox_name );

			so_subscribe( mbox )
				.in( so_default_state() )
					.event( &a_ordinary_t::some_handler );

			// Give time to test message sender.
			ACE_OS::sleep( ACE_Time_Value( 0, 10*1000 ) );
		}

		virtual void
		so_evt_start();

		void
		some_handler(
			const so_5::rt::event_data_t< some_message > & msg );
};

void
a_ordinary_t::so_evt_start()
{
	++g_evt_count;
}

void
a_ordinary_t::some_handler(
	const so_5::rt::event_data_t< some_message > & msg )
{
	++g_evt_count;
}

class a_throwing_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:

		a_throwing_t(
			so_5::rt::so_environment_t & env )
			:
				base_type_t( env )
		{
			++g_agents_count;
		}

		virtual ~a_throwing_t()
		{
			--g_agents_count;
		}

		virtual void
		so_define_agent()
		{
		}

		virtual void
		so_evt_start();
};

class throwing_disp_binder_t
	:
		public so_5::rt::disp_binder_t
{
	public:
		throwing_disp_binder_t() {}
		virtual ~throwing_disp_binder_t() {}

		virtual void
		bind_agent(
			so_5::rt::impl::so_environment_impl_t & env,
			so_5::rt::agent_ref_t & agent_ref )
		{
			throw std::runtime_error(
				"throwing while binding agent to disp" );
		}

		virtual void
		unbind_agent(
			so_5::rt::impl::so_environment_impl_t & env,
			so_5::rt::agent_ref_t & agent_ref )
		{
		}

};

void
a_throwing_t::so_evt_start()
{
	// This method should not be called.
	std::cerr << "error: a_throwing_t::so_evt_start called.";
	std::abort();
}

// An agent which send message.
class a_message_sender_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:

		a_message_sender_t( so_5::rt::so_environment_t & env )
			:
				base_type_t( env )
		{}

		virtual ~a_message_sender_t()
		{}

		virtual void
		so_define_agent()
		{}

		virtual void
		so_evt_start()
		{
			so_5::rt::mbox_ref_t mbox =
				so_environment().create_local_mbox( g_test_mbox_name );

			ACE_Time_Value time_limit = ACE_OS::gettimeofday();
			time_limit += ACE_Time_Value( 0, 150*1000 );

			int msg_cnt = 0;
			while( time_limit > ACE_OS::gettimeofday() )
			{
				mbox->deliver_message< some_message >();
				++msg_cnt;
			}
		}
};

void
reg_message_sender(
	so_5::rt::so_environment_t & env )
{
	so_5::rt::agent_coop_unique_ptr_t coop =
		env.create_coop(
			"message_sender_coop",
			so_5::disp::active_obj::create_disp_binder(
				"active_obj" ) );

	coop->add_agent(
		so_5::rt::agent_ref_t( new a_message_sender_t( env ) ) );
}

void
reg_coop(
	so_5::rt::so_environment_t & env )
{
	so_5::rt::agent_coop_unique_ptr_t coop =
		env.create_coop( "test_coop" );

	coop->add_agent( so_5::rt::agent_ref_t(
		new a_ordinary_t( env ) ) );
	coop->add_agent( so_5::rt::agent_ref_t(
		new a_ordinary_t( env ) ) );
	coop->add_agent( so_5::rt::agent_ref_t(
		new a_ordinary_t( env ) ) );
	coop->add_agent( so_5::rt::agent_ref_t(
		new a_ordinary_t( env ) ) );
	coop->add_agent( so_5::rt::agent_ref_t(
		new a_ordinary_t( env ) ) );

	// This agent will throw an exception during binding for dispatcher.
	coop->add_agent(
		so_5::rt::agent_ref_t( new a_throwing_t( env ) ),
		so_5::rt::disp_binder_unique_ptr_t(
			new throwing_disp_binder_t ) );

	coop->add_agent( so_5::rt::agent_ref_t(
		new a_ordinary_t( env ) ) );
	coop->add_agent( so_5::rt::agent_ref_t(
		new a_ordinary_t( env ) ) );
	coop->add_agent( so_5::rt::agent_ref_t(
		new a_ordinary_t( env ) ) );
	coop->add_agent( so_5::rt::agent_ref_t(
		new a_ordinary_t( env ) ) );

	env.register_coop(
		std::move( coop ),
		so_5::DO_NOT_THROW_ON_ERROR );
}

void
init( so_5::rt::so_environment_t & env )
{
	reg_message_sender( env );
	reg_coop( env );

	env.stop();
}

int
main( int argc, char * argv[] )
{
	try
	{
		so_5::api::run_so_environment(
			&init,
			so_5::rt::so_environment_params_t()
				.mbox_mutex_pool_size( 4 )
				.agent_event_queue_mutex_pool_size( 4 )
				.add_named_dispatcher(
					"active_obj",
					so_5::disp::active_obj::create_disp() ) );

		if( 0 != g_agents_count.value() )
		{
			std::cerr << "g_agents_count: "
				<< g_agents_count.value() << "\n";
			throw std::runtime_error( "g_agents_count != 0" );
		}

		std::cout << "event handled: " << g_evt_count.value() << "\n";
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}