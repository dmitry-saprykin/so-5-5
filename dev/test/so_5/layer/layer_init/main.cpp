/*
 * A test for checking layer initialization.
 */

#include <iostream>
#include <map>
#include <exception>

#include <cpp_util_2/h/defs.hpp>

#include <so_5/api/h/api.hpp>
#include <so_5/rt/h/rt.hpp>
#include <so_5/rt/h/so_layer.hpp>

#include <utest_helper_1/h/helper.hpp>

//
// test_layer_t
//

class test_layer_t
	:
		public so_5::rt::so_layer_t
{
	private:
		int m_op_seq_counter;

		enum operations_t
		{
			OP_START = 0,
			OP_SHUTDOWN = 1,
			OP_WAIT = 2
		};

		static int op_calls[3];

	public:
		test_layer_t()
			:
				m_op_seq_counter( 0 )
		{}

		virtual ~test_layer_t()
		{}

		virtual so_5::ret_code_t
		start()
		{
			op_calls[ OP_START ] = m_op_seq_counter++;

			return 0;
		}

		virtual void
		shutdown()
		{
			op_calls[ OP_SHUTDOWN ] = m_op_seq_counter++;
		}

		virtual void
		wait()
		{
			op_calls[ OP_WAIT ] = m_op_seq_counter++;
		}

	static void
	check_calls();

};

int test_layer_t::op_calls[3] = { -1, -1, -1 };
void
test_layer_t::check_calls()
{
	UT_CHECK_EQ( op_calls[ OP_START ], OP_START );
	UT_CHECK_EQ( op_calls[ OP_SHUTDOWN ], OP_SHUTDOWN );
	UT_CHECK_EQ( op_calls[ OP_WAIT ], OP_WAIT );
}


void
init( so_5::rt::so_environment_t & env )
{
	env.stop();
}

UT_UNIT_TEST( check_layer_lifecircle_op_calls )
{
	so_5::api::run_so_environment(
			&init,
			so_5::rt::so_environment_params_t()
				.mbox_mutex_pool_size( 4 )
				.agent_event_queue_mutex_pool_size( 4 )
				.add_layer(
					std::unique_ptr< test_layer_t >(
						new test_layer_t ) ) );

	test_layer_t::check_calls();
}

int
main( int, char ** )
{
	UT_RUN_UNIT_TEST( check_layer_lifecircle_op_calls );

	return 0;
}