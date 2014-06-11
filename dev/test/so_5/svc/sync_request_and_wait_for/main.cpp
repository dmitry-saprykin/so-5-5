/*
 * A test for time-limited synchronous service request calls.
 */

#include <iostream>
#include <exception>
#include <sstream>
#include <chrono>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>
#include <so_5/h/types.hpp>

#include <so_5/disp/active_obj/h/pub.hpp>

#include "../a_time_sentinel.hpp"

struct msg_get_default : public so_5::rt::signal_t
	{};

struct msg_back_call_get_default : public so_5::rt::signal_t
	{};

struct msg_convert : public so_5::rt::message_t
	{
		int m_value;

		msg_convert( int value ) : m_value( value )
			{}
	};

struct msg_back_call_convert : public so_5::rt::message_t
	{
		int m_value;

		msg_back_call_convert( int value ) : m_value( value )
			{}
	};

struct msg_back_call : public so_5::rt::signal_t
	{};

class a_convert_service_t
	:	public so_5::rt::agent_t
	{
	public :
		a_convert_service_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & self_mbox,
			const so_5::rt::mbox_ref_t & back_call_mbox )
			:	so_5::rt::agent_t( env )
			,	m_self_mbox( self_mbox )
			,	m_back_call_mbox( back_call_mbox )
			{}

		virtual so_5::rt::exception_reaction_t
		so_exception_reaction() const
			{
				return so_5::rt::abort_on_exception;
			}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_self_mbox )
						.event( &a_convert_service_t::svc_default );

				so_subscribe( m_self_mbox )
						.event( &a_convert_service_t::svc_back_call_default );

				so_subscribe( m_self_mbox )
						.event( &a_convert_service_t::svc_convert );

				so_subscribe( m_self_mbox )
						.event( &a_convert_service_t::svc_back_call_convert );
			}

		std::string
		svc_default( const so_5::rt::event_data_t< msg_get_default > & )
			{
				return "DEFAULT";
			}

		std::string
		svc_back_call_default(
			const so_5::rt::event_data_t< msg_back_call_get_default > & )
			{
				m_back_call_mbox->get_one< void >()
						.wait_forever().request< msg_back_call >();

				return "NOT USED DEFAULT";
			}

		std::string
		svc_convert( const so_5::rt::event_data_t< msg_convert > & evt )
			{
				std::ostringstream s;
				s << evt->m_value;

				return s.str();
			}

		std::string
		svc_back_call_convert(
			const so_5::rt::event_data_t< msg_back_call_convert > & evt )
			{
				m_back_call_mbox->get_one< void >()
						.wait_forever().request< msg_back_call >();

				return "NOT USED";
			}

	private :
		const so_5::rt::mbox_ref_t m_self_mbox;
		const so_5::rt::mbox_ref_t m_back_call_mbox;
	};

void
compare_and_abort_if_missmatch(
	const std::string & actual,
	const std::string & expected )
	{
		if( actual != expected )
			{
				std::cerr << "VALUE MISSMATCH: actual='"
						<< actual << "', expected='" << expected << "'"
						<< std::endl;
				std::abort();
			}
	}

class a_client_t
	:	public so_5::rt::agent_t
	{
	public :
		struct msg_next_convert : public so_5::rt::signal_t {};

		a_client_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & self_mbox,
			const so_5::rt::mbox_ref_t & svc_mbox )
			:	so_5::rt::agent_t( env )
			,	st_deadlocks( self_ptr(), "st_deadlocks" )
			,	m_self_mbox( self_mbox )
			,	m_svc_mbox( svc_mbox )
			,	m_normal_convert_actions_current( 0 )
			,	m_back_call_actions_current( 0 )
			{
				fill_normal_actions();
				fill_back_call_actions();
			}

		virtual so_5::rt::exception_reaction_t
		so_exception_reaction() const
			{
				return so_5::rt::abort_on_exception;
			}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_self_mbox ).event(
						&a_client_t::evt_next_normal_convert );

				so_subscribe( m_self_mbox ).in( st_deadlocks )
						.event( &a_client_t::evt_next_back_call_convert );

				so_subscribe( m_self_mbox ).in( st_deadlocks )
						.event( &a_client_t::svc_back_call );
			}

		virtual void
		so_evt_start()
			{
				m_self_mbox->deliver_signal< msg_next_convert >();
			}

		void
		evt_next_normal_convert(
			const so_5::rt::event_data_t< msg_next_convert > & )
			{
				if( m_normal_convert_actions_current <
						m_normal_convert_actions.size() )
					{
						m_normal_convert_actions[ m_normal_convert_actions_current ]();
						++m_normal_convert_actions_current;
					}
				else
					{
						so_change_state( st_deadlocks );
					}

				m_self_mbox->deliver_signal< msg_next_convert >();
			}

		void
		evt_next_back_call_convert(
			const so_5::rt::event_data_t< msg_next_convert > & )
			{
				if( m_back_call_actions_current <
						m_back_call_actions.size() )
					{
						m_back_call_actions[ m_back_call_actions_current ]();
						++m_back_call_actions_current;

						m_self_mbox->deliver_signal< msg_next_convert >();
					}
				else
					{
						so_environment().stop();
					}
			}
		void
		svc_back_call( const so_5::rt::event_data_t< msg_back_call > & )
			{
			}

	private :
		const so_5::rt::state_t st_deadlocks;

		const so_5::rt::mbox_ref_t m_self_mbox;
		const so_5::rt::mbox_ref_t m_svc_mbox;

		typedef std::function< void(void) > action_t;

		typedef std::vector< action_t > action_container_t;

		action_container_t m_normal_convert_actions;
		action_container_t::size_type m_normal_convert_actions_current;

		action_container_t m_back_call_actions;
		action_container_t::size_type m_back_call_actions_current;

		void
		fill_normal_actions()
			{
				m_normal_convert_actions.emplace_back(
						[this]() {
							compare_and_abort_if_missmatch(
									m_svc_mbox->get_one< std::string >()
											.wait_forever()
											.request< msg_get_default >(),
									"DEFAULT" );
							} );

				m_normal_convert_actions.emplace_back(
						[this]() {
							compare_and_abort_if_missmatch(
									m_svc_mbox->get_one< std::string >()
											.wait_forever()
											.request( new msg_convert(1) ),
									"1" );
							} );

				m_normal_convert_actions.emplace_back(
						[this]() {
							compare_and_abort_if_missmatch(
									m_svc_mbox->get_one< std::string >()
											.wait_forever()
											.request(
													std::unique_ptr< msg_convert >(
															new msg_convert(2) ) ),
									"2" );
							} );

#if !defined( SO_5_NO_VARIADIC_TEMPLATES )
				m_normal_convert_actions.emplace_back(
						[this]() {
							compare_and_abort_if_missmatch(
									m_svc_mbox->get_one< std::string >()
											.wait_forever()
											.make_request< msg_convert >( 3 ),
									"3" );
							} );
#endif
			}

		void
		fill_back_call_actions()
			{
				m_back_call_actions.emplace_back( make_exception_handling_envelope(
						[this]() {
							m_svc_mbox->get_one< std::string >()
									.wait_for( std::chrono::milliseconds( 50 ) )
									.request< msg_back_call_get_default >();
						}, "get_default" ) );

				m_back_call_actions.emplace_back( make_exception_handling_envelope(
						[this]() {
							m_svc_mbox->get_one< std::string >()
									.wait_for( std::chrono::milliseconds( 50 ) )
									.request( new msg_back_call_convert(11) );
						}, "11" ) );

				m_back_call_actions.emplace_back( make_exception_handling_envelope(
						[this]() {
							m_svc_mbox->get_one< std::string >()
									.wait_for( std::chrono::milliseconds( 50 ) )
									.request(
											std::unique_ptr< msg_back_call_convert >(
													new msg_back_call_convert(12) ) );
						}, "12" ) );

#if !defined( SO_5_NO_VARIADIC_TEMPLATES )
				m_back_call_actions.emplace_back( make_exception_handling_envelope(
						[this]() {
							m_svc_mbox->get_one< std::string >()
									.wait_for( std::chrono::milliseconds( 50 ) )
									.make_request< msg_back_call_convert >( 13 );
						}, "13" ) );
#endif
			}

		static action_t
		make_exception_handling_envelope(
			action_t action,
			const std::string & description )
			{
				return [action, description]() {
					try
						{
							action();

							std::cerr << "an exception expected for the case: "
									<< description << std::endl;

							std::abort();
						}
					catch( const so_5::exception_t & x )
						{
							if( so_5::rc_svc_result_not_received_yet != x.error_code() )
								{
									std::cerr << "test case: '" << description << "', "
											<< "expected error_code: "
											<< so_5::rc_svc_result_not_received_yet
											<< ", actual error_code: "
											<< x.error_code()
											<< std::endl;
									std::abort();
								}
						}
				};
			}
	};

void
init(
	so_5::rt::so_environment_t & env )
	{
		auto coop = env.create_coop(
				so_5::rt::nonempty_name_t( "test_coop" ),
				so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

		auto back_call_mbox = env.create_local_mbox();
		auto svc_mbox = env.create_local_mbox();

		coop->add_agent( new a_convert_service_t(
				env, svc_mbox, back_call_mbox ) );
		coop->add_agent( new a_client_t( env, back_call_mbox, svc_mbox ) );
		coop->add_agent( new a_time_sentinel_t( env ) );

		env.register_coop( std::move( coop ) );
	}

int
main( int, char ** )
	{
		try
			{
				so_5::api::run_so_environment(
					&init,
					std::move(
						so_5::rt::so_environment_params_t()
							.add_named_dispatcher(
								so_5::rt::nonempty_name_t( "active_obj" ),
								so_5::disp::active_obj::create_disp() ) ) );
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}
