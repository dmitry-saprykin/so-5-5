/*
 * A test for receiving different messages.
 */

#include <iostream>
#include <exception>
#include <stdexcept>

#include <ace/OS.h>
#include <ace/Time_Value.h>

#include <so_5/h/types.hpp>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

struct test_message_1
	:
		public so_5::rt::message_t
{
	test_message_1(): m_year_1( 2010 ), m_year_2( 2011 ) {}

	int m_year_1;
	int m_year_2;
};

struct test_message_2
	:
		public so_5::rt::message_t
{
	test_message_2(): m_so( "SObjectizer" ), m_ver( "5" ) {}

	std::string m_so;
	std::string m_ver;
};

struct test_message_3
	:
		public so_5::rt::message_t
{
	test_message_3(): m_where( "Gomel" ) {}

	std::string m_where;
};

class test_agent_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:
		test_agent_t(
			so_5::rt::so_environment_t & env )
			:
				base_type_t( env ),
				m_test_mbox( so_environment().create_local_mbox() )
		{}

		virtual ~test_agent_t()
		{}

		virtual void
		so_define_agent();

		virtual void
		so_evt_start();

		void
		evt_test_1(
			const so_5::rt::event_data_t< test_message_1 > &
				msg );

		void
		evt_test_2(
			const so_5::rt::event_data_t< test_message_2 > &
				msg );

		void
		evt_test_3(
			const so_5::rt::event_data_t< test_message_3 > &
				msg );

	private:
		so_5::rt::mbox_ref_t m_test_mbox;
};

void
test_agent_t::so_define_agent()
{
	so_subscribe( m_test_mbox )
		.event(
			&test_agent_t::evt_test_1,
			so_5::THROW_ON_ERROR );

	so_subscribe( m_test_mbox )
		.event(
			&test_agent_t::evt_test_2,
			so_5::THROW_ON_ERROR );

	so_subscribe( m_test_mbox )
		.event(
			&test_agent_t::evt_test_3,
			so_5::THROW_ON_ERROR );
}

void
test_agent_t::so_evt_start()
{
	m_test_mbox->deliver_message(
		std::unique_ptr< test_message_1 >(
			new test_message_1 ) );

	m_test_mbox->deliver_message(
		std::unique_ptr< test_message_2 >(
			new test_message_2 ) );

	m_test_mbox->deliver_message(
		std::unique_ptr< test_message_3 >(
			new test_message_3 ) );
}

void
test_agent_t::evt_test_1(
	const so_5::rt::event_data_t< test_message_1 > &
		msg )
{
	if( 0 == msg.get() )
		throw std::runtime_error(
			"evt_test_1: 0 == msg.get()" );

	if( msg->m_year_1 != 2010 || msg->m_year_2 != 2011 )
	{
		throw std::runtime_error(
			"evt_test_1: "
			"msg->m_year_1 != 2010 || msg->m_year_2 != 2011" );
	}
}

void
test_agent_t::evt_test_2(
	const so_5::rt::event_data_t< test_message_2 > &
		msg )
{
	if( 0 == msg.get() )
		throw std::runtime_error(
			"evt_test_2: 0 == msg.get()" );

	if( msg->m_so !="SObjectizer" || msg->m_ver != "5" )
	{
		throw std::runtime_error(
			"evt_test_2: "
			"msg->m_so !=\"SObjectizer\" || msg->m_ver != \"5\"" );
	}
}

void
test_agent_t::evt_test_3(
	const so_5::rt::event_data_t< test_message_3 > &
		msg )
{
	if( 0 == msg.get() )
		throw std::runtime_error(
			"evt_test_3: 0 == msg.get()" );

	if( msg->m_where !="Gomel" )
	{
		throw std::runtime_error(
			"evt_test_3: "
			"msg->m_where !=\"Gomel\"" );
	}

	so_environment().stop();
}

void
init( so_5::rt::so_environment_t & env )
{
	so_5::rt::agent_coop_unique_ptr_t coop =
		env.create_coop( "test_coop" );

	coop->add_agent(
		so_5::rt::agent_ref_t(
			new test_agent_t( env ) ) );

	env.register_coop(
		std::move( coop ),
		so_5::THROW_ON_ERROR );
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
				.agent_event_queue_mutex_pool_size( 4 ) );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
