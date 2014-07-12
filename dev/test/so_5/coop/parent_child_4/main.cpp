/*
 * A test for inability to deregister parent cooperation during
 * child cooperation registration.
 */

#include <iostream>
#include <sstream>

#include <ace/OS.h>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

#include <so_5/disp/active_obj/h/pub.hpp>

#include "test/so_5/svc/a_time_sentinel.hpp"

struct msg_parent_started : public so_5::rt::signal_t {};

struct msg_initiate_dereg : public so_5::rt::signal_t {};

struct msg_check_signal : public so_5::rt::signal_t {};

struct msg_shutdown : public so_5::rt::signal_t {};

class a_child_t : public so_5::rt::agent_t
{
	public :
		a_child_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & self_mbox,
			const so_5::rt::mbox_ref_t & parent_mbox )
			:	so_5::rt::agent_t( env )
			,	m_mbox( self_mbox )
			,	m_parent_mbox( parent_mbox )
			,	m_check_signal_received( false )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe( m_mbox ).event(
					so_5::signal< msg_check_signal >,
					&a_child_t::evt_check_signal );

			try
			{
				m_parent_mbox->run_one()
						.wait_for( std::chrono::milliseconds( 100 ) )
						.sync_get< msg_initiate_dereg >();

				throw std::runtime_error( "timeout expected" );
			}
			catch( const so_5::exception_t & x )
			{
				if( so_5::rc_svc_result_not_received_yet != x.error_code() )
				{
					std::cerr << "timeout expiration expected, but "
							"the actual error_code: "
							<< x.error_code() << std::endl;
					std::abort();
				}
			}

			m_mbox->deliver_signal< msg_check_signal >();
		}

		virtual void
		so_evt_finish()
		{
			if( !m_check_signal_received )
			{
				std::cerr << "so_evt_finish before check_signal" << std::endl;
				std::abort();
			}
		}

		virtual void
		evt_check_signal()
		{
			m_check_signal_received = true;
		}

	private :
		const so_5::rt::mbox_ref_t m_mbox;
		const so_5::rt::mbox_ref_t m_parent_mbox;

		bool m_check_signal_received;
};

class a_parent_t : public so_5::rt::agent_t
{
	public :
		a_parent_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & mbox )
			:	so_5::rt::agent_t( env )
			,	m_mbox( mbox )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe( m_mbox ).event( so_5::signal< msg_initiate_dereg >,
					&a_parent_t::evt_initiate_dereg );
		}

		virtual void
		so_evt_start()
		{
			m_mbox->deliver_signal< msg_parent_started >();
		}

		virtual void
		evt_initiate_dereg()
		{
			so_environment().deregister_coop( so_coop_name(),
					so_5::rt::dereg_reason::normal );
		}

	private :
		const so_5::rt::mbox_ref_t m_mbox;
};

class a_driver_t : public so_5::rt::agent_t
{
	public :
		a_driver_t(
			so_5::rt::so_environment_t & env )
			:	so_5::rt::agent_t( env )
			,	m_mbox( env.create_local_mbox() )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe( m_mbox ).event( so_5::signal< msg_parent_started >,
					&a_driver_t::evt_parent_started );

			so_subscribe( m_mbox ).event( so_5::signal< msg_shutdown >,
					&a_driver_t::evt_shutdown );
		}

		virtual void
		so_evt_start()
		{
			so_environment().register_agent_as_coop(
				"parent",
				new a_parent_t( so_environment(), m_mbox ),
				so_5::disp::active_obj::create_disp_binder( "active_obj" ) );
		}

		void
		evt_parent_started()
		{
			auto coop = so_environment().create_coop( "child",
					so_5::disp::active_obj::create_disp_binder( "active_obj" ) );
			coop->set_parent_coop_name( "parent" );

			coop->add_agent(
				new a_child_t(
					so_environment(),
					so_environment().create_local_mbox(),
					m_mbox ) );

			so_environment().register_coop( std::move( coop ) );

			so_environment().single_timer< msg_shutdown >( m_mbox, 500 );
		}

		void
		evt_shutdown()
		{
			so_environment().stop();
		}

	private :
		const so_5::rt::mbox_ref_t m_mbox;
};

void
init( so_5::rt::so_environment_t & env )
{
	auto coop = env.create_coop( "driver",
			so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

	coop->add_agent( new a_driver_t( env ) );
	coop->add_agent( new a_time_sentinel_t( env ) );

	env.register_coop( std::move( coop ) );
}

int
main( int argc, char * argv[] )
{
	try
	{
		so_5::api::run_so_environment(
				&init,
				[]( so_5::rt::so_environment_params_t & p ) {
					p.add_named_dispatcher( "active_obj",
						so_5::disp::active_obj::create_disp() );
				} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

