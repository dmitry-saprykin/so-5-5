/*
 * A sample for use service handlers for parallel sum of vector.
 */

#include <iostream>
#include <exception>
#include <numeric>
#include <vector>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>
#include <so_5/h/types.hpp>

#include <so_5/disp/active_obj/h/pub.hpp>

typedef std::vector< int > vector_t;

struct msg_sum_part : public so_5::rt::message_t
	{
		vector_t::const_iterator m_begin;
		vector_t::const_iterator m_end;

		msg_sum_part(
			const vector_t::const_iterator & b,
			const vector_t::const_iterator & e )
			:	m_begin( b )
			,	m_end( e )
			{}
	};

struct msg_sum_vector : public so_5::rt::message_t
	{
		const vector_t & m_vector;

		msg_sum_vector( const vector_t & v ) : m_vector( v )
			{}
	};

class a_part_summator_t : public so_5::rt::agent_t
	{
	public :
		a_part_summator_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & self_mbox )
			:	so_5::rt::agent_t( env )
			,	m_self_mbox( self_mbox )
			{}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_self_mbox ).event(
						[]( const msg_sum_part & part ) {
							return std::accumulate( part.m_begin, part.m_end, 0 );
						} );
			}

	private :
		const so_5::rt::mbox_ref_t m_self_mbox;
	};

class a_vector_summator_t : public so_5::rt::agent_t
	{
	public :
		a_vector_summator_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & self_mbox )
			:	so_5::rt::agent_t( env )
			,	m_self_mbox( self_mbox )
			,	m_part_summator_mbox( env.create_local_mbox() )
			{}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_self_mbox ).event( &a_vector_summator_t::evt_sum );
			}

		virtual void
		so_evt_start()
			{
				auto coop = so_environment().create_coop(
						so_coop_name() + "::part_summator",
						so_5::disp::active_obj::create_disp_binder( "active_obj" ) );
				coop->set_parent_coop_name( so_coop_name() );

				coop->add_agent(
						new a_part_summator_t(
								so_environment(),
								m_part_summator_mbox ) );

				so_environment().register_coop( std::move( coop ) );
			}

		int
		evt_sum( const msg_sum_vector & evt )
			{
				auto m = evt.m_vector.begin() + evt.m_vector.size() / 2;

				return std::accumulate( evt.m_vector.begin(), m, 0 ) +
						m_part_summator_mbox->get_one< int >().wait_forever().sync_get(
								new msg_sum_part( m, evt.m_vector.end() ) );
			}

	private :
		const so_5::rt::mbox_ref_t m_self_mbox;
		const so_5::rt::mbox_ref_t m_part_summator_mbox;
	};

class progress_indicator_t
	{
	public :
		progress_indicator_t( std::size_t total )
			:	m_total( total )
			,	m_percents( 0 )
			{}

		~progress_indicator_t()
			{
				std::cout << std::endl;
			}

		void
		update( std::size_t current )
			{
				int p = static_cast< int >(
						(double(current + 1) / double(m_total)) * 100);
				if( p != m_percents )
					{
						m_percents = p;
						std::cout << m_percents << "\r" << std::flush;
					}
			}

	private :
		const std::size_t m_total;
		int m_percents;
	};

class a_runner_t : public so_5::rt::agent_t
	{
	public :
		a_runner_t(
			so_5::rt::so_environment_t & env,
			std::size_t iterations )
			:	so_5::rt::agent_t( env )
			,	ITERATIONS( iterations )
			,	m_summator_mbox( env.create_local_mbox() )
			{}

		virtual void
		so_evt_start()
			{
				create_summator_coop();
				fill_test_vector();

				do_calculations();

				so_environment().stop();
			}

	private :
		const std::size_t ITERATIONS;

		const so_5::rt::mbox_ref_t m_summator_mbox;

		vector_t m_vector;

		void
		create_summator_coop()
			{
				auto coop = so_environment().create_coop(
						so_coop_name() + "::vector_summator",
						so_5::disp::active_obj::create_disp_binder( "active_obj" ) );
				coop->set_parent_coop_name( so_coop_name() );

				coop->add_agent(
						new a_vector_summator_t(
								so_environment(),
								m_summator_mbox ) );

				so_environment().register_coop( std::move( coop ) );
			}

		void
		fill_test_vector()
			{
				const std::size_t CAPACITY = 1000;
				m_vector.reserve( CAPACITY );

				for( int i = 0; i != static_cast< int >(CAPACITY); ++i )
					m_vector.push_back( i );
			}

		void
		do_calculations()
			{
				progress_indicator_t indicator( (ITERATIONS) );

				for( std::size_t i = 0; i != ITERATIONS; ++i )
					{
						m_summator_mbox->get_one< int >()
								.wait_forever()
								.sync_get( new msg_sum_vector( m_vector ) );

						indicator.update( i );
					}
			}
	};

void
init(
	so_5::rt::so_environment_t & env,
	std::size_t iterations )
	{
		auto coop = env.create_coop(
				so_5::rt::nonempty_name_t( "test_coop" ),
				so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

		coop->add_agent( new a_runner_t( env, iterations ) );

		env.register_coop( std::move( coop ) );
	}

int
main( int argc, char ** argv )
	{
		try
			{
				const std::size_t ITERATIONS = 2 == argc ?
						static_cast< std::size_t >(std::atoi( argv[1] )) : 10u;

				so_5::api::run_so_environment_with_parameter(
						&init,
						ITERATIONS,
						std::move(
								so_5::rt::so_environment_params_t()
										.add_named_dispatcher(
												"active_obj",
												so_5::disp::active_obj::create_disp() ) ) );
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}
