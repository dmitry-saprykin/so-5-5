/*
 * A simple implementation of chameneos benchmark (this implementation is
 * based on definition which was used in The Great Language Shootout Gate
 * in 2007).
 *
 * There are four chameneos with different colors.
 * There is a meeting place for them.
 *
 * Each creature is trying to go to the meeting place. Only two of them
 * could do that at the same time. During the meeting they should change
 * their colors by special rule. Then they should leave the meeting place
 * and do the next attempt to go to the meeting place again.
 *
 * There is a limitation for meeting count. When this limit is reached
 * every creature should receive a special color FADED and report count of
 * other creatures met.
 *
 * Total count of meetings should be reported at the end of the test.
 *
 * This sample is implemented here with two different types of agents:
 * - the first one is the type of meeting place. Agent of that type does
 *   several task. It handles meetings of creatures and count meetings.
 *   When the limit of meeting is reached that agent inform all creatures
 *   about test shutdown. Then the agent receives shutdown acknowledgements
 *   from creatures and calculates total meeting count;
 * - the second one is the type of creature. Agents of that type are trying
 *   to reach meeting place. They send meeting requests to meeting place agent
 *   and handle meeting result or shutdown signal.
 */

#include <iostream>
#include <iterator>
#include <numeric>

#include <ace/OS.h>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

#include <so_5/disp/active_obj/h/pub.hpp>

enum color_t
	{
		BLUE = 0,
		RED = 1,
		YELLOW = 2,
		FADED = 3
	};

struct msg_meeting_result : public so_5::rt::message_t
	{
		color_t m_color;

		msg_meeting_result( color_t color )
			:	m_color( color )
			{}
	};

typedef so_5::rt::smart_atomic_reference_t< msg_meeting_result >
	msg_meeting_result_smart_ref_t;

struct msg_meeting_request : public so_5::rt::message_t
	{
		so_5::rt::mbox_ref_t m_who;
		color_t m_color;

		msg_meeting_result_smart_ref_t m_result_message;

		msg_meeting_request(
			const so_5::rt::mbox_ref_t & who,
			color_t color,
			msg_meeting_result_smart_ref_t result_message )
			:	m_who( who )
			,	m_color( color )
			,	m_result_message( result_message )
			{}
	};

typedef so_5::rt::smart_atomic_reference_t< msg_meeting_request >
	msg_meeting_request_smart_ref_t;

struct msg_shutdown_request : public so_5::rt::signal_t {};

struct msg_shutdown_ack : public so_5::rt::message_t
	{
		int m_creatures_met;

		msg_shutdown_ack( int creatures_met )
			:	m_creatures_met( creatures_met )
			{}
	};

class a_meeting_place_t
	:	public so_5::rt::agent_t
	{
	public :
		a_meeting_place_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & self_mbox,
			int creatures,
			int meetings )
			:	so_5::rt::agent_t( env )
			,	st_empty( self_ptr(), "st_empty" )
			,	st_one_creature_inside( self_ptr(), "st_one_creature_inside" )
			,	m_self_mbox( self_mbox )
			,	m_creatures_alive( creatures )	
			,	m_remaining_meetings( meetings )
			,	m_total_meetings( 0 )
			{}

		virtual void
		so_define_agent()
			{
				so_change_state( st_empty );

				so_subscribe( m_self_mbox ).in( st_empty )
					.event( &a_meeting_place_t::evt_first_creature );
				so_subscribe( m_self_mbox ).in( st_one_creature_inside )
					.event( &a_meeting_place_t::evt_second_creature );

				so_subscribe( m_self_mbox ).in( st_empty )
					.event( &a_meeting_place_t::evt_shutdown_ack );
			}

		void
		evt_first_creature(
			const so_5::rt::event_data_t< msg_meeting_request > & evt )
			{
				if( m_remaining_meetings )
				{
					so_change_state( st_one_creature_inside );

					m_first_creature_info = evt.make_reference();
				}
				else
					evt->m_who->deliver_signal< msg_shutdown_request >();
			}

		void
		evt_second_creature(
			const msg_meeting_request & evt )
			{
				evt.m_result_message->m_color =
						m_first_creature_info->m_color;
				m_first_creature_info->m_result_message->m_color = evt.m_color;

				evt.m_who->deliver_message( evt.m_result_message );
				m_first_creature_info->m_who->deliver_message(
						m_first_creature_info->m_result_message );

				m_first_creature_info.reset();

				--m_remaining_meetings;

				so_change_state( st_empty );
			}

		void
		evt_shutdown_ack(
			const msg_shutdown_ack & evt )
			{
				m_total_meetings += evt.m_creatures_met;
				
				if( 0 >= --m_creatures_alive )
				{
					std::cout << "Total: " << m_total_meetings << std::endl;

					so_environment().stop();
				}
			}

	private :
		so_5::rt::state_t st_empty;
		so_5::rt::state_t st_one_creature_inside;

		const so_5::rt::mbox_ref_t m_self_mbox;

		int m_creatures_alive;
		int m_remaining_meetings;
		int m_total_meetings;

		msg_meeting_request_smart_ref_t m_first_creature_info;
	};

class a_creature_t
	:	public so_5::rt::agent_t
	{
	public :
		a_creature_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & meeting_place_mbox,
			color_t color )
			:	so_5::rt::agent_t( env )
			,	m_meeting_place_mbox( meeting_place_mbox )
			,	m_self_mbox( env.create_local_mbox() )
			,	m_meeting_counter( 0 )
			,	m_response_message( new msg_meeting_result( color ) )
			,	m_request_message( new msg_meeting_request(
						m_self_mbox,
						color,
						m_response_message ) )
			{}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_self_mbox )
					.event( &a_creature_t::evt_meeting_result );

				so_subscribe( m_self_mbox )
					.event(
							so_5::signal< msg_shutdown_request >,
							&a_creature_t::evt_shutdown_request );
			}

		virtual void
		so_evt_start()
			{
				m_meeting_place_mbox->deliver_message( m_request_message );
			}

		void
		evt_meeting_result(
			const msg_meeting_result & evt )
			{
				m_request_message->m_color = complement( evt.m_color );
				m_meeting_counter++;

				m_meeting_place_mbox->deliver_message( m_request_message );
			}

		void
		evt_shutdown_request()
			{
				m_request_message->m_color = FADED;
				std::cout << "Creatures met: " << m_meeting_counter << std::endl;

				m_meeting_place_mbox->deliver_message(
						new msg_shutdown_ack( m_meeting_counter ) );
			}

	private :
		const so_5::rt::mbox_ref_t m_meeting_place_mbox;
		const so_5::rt::mbox_ref_t m_self_mbox;

		int m_meeting_counter;

		msg_meeting_result_smart_ref_t m_response_message;
		msg_meeting_request_smart_ref_t m_request_message;

		color_t
		complement( color_t other ) const
			{
				switch( m_request_message->m_color )
					{
					case BLUE:
						return other == RED ? YELLOW : RED;
					case RED:
						return other == BLUE ? YELLOW : BLUE;
					case YELLOW:
						return other == BLUE ? RED : BLUE;
					default:
						break;
					}
				return m_request_message->m_color;
			}
	};

const std::size_t CREATURE_COUNT = 4;

void
init(
	so_5::rt::so_environment_t & env,
	int meetings )
	{
		color_t creature_colors[ CREATURE_COUNT ] =
			{ BLUE, RED, YELLOW, BLUE };

		auto meeting_place_mbox = env.create_local_mbox();

		auto coop = env.create_coop( "chameneos",
				so_5::disp::active_obj::create_disp_binder(
						"active_obj" ) );

		coop->add_agent(
				new a_meeting_place_t(
						env,
						meeting_place_mbox,
						CREATURE_COUNT,
						meetings ) );
		
		for( std::size_t i = 0; i != CREATURE_COUNT; ++i )
			{
				coop->add_agent(
						new a_creature_t(
								env,
								meeting_place_mbox,
								creature_colors[ i ] ) );
			}

		env.register_coop( std::move( coop ) );
	}

int
main( int argc, char ** argv )
{
	try
	{
		so_5::api::run_so_environment(
				[argc, argv]( so_5::rt::so_environment_t & env ) {
					const int meetings = 2 == argc ? std::atoi( argv[1] ) : 10;
					init( env, meetings );
				},
				[]( so_5::rt::so_environment_params_t & p ) {
					p.add_named_dispatcher(
							"active_obj",
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
