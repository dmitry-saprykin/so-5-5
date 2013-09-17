/*
	SObjectizer 5.
*/

#include <so_5/disp/one_thread/impl/h/disp.hpp>

namespace so_5
{

namespace disp
{

namespace one_thread
{

namespace impl
{

dispatcher_t::dispatcher_t()
	:
		m_work_thread( *self_ptr() )
{
}

dispatcher_t::~dispatcher_t()
{
}

ret_code_t
dispatcher_t::start()
{
	m_work_thread.start();
	return 0;
}

void
dispatcher_t::shutdown()
{
	m_work_thread.shutdown();
}

void
dispatcher_t::wait()
{
	m_work_thread.wait();
}

void
dispatcher_t::put_event_execution_request(
	const so_5::rt::agent_ref_t & agent_ref,
	unsigned int event_count )
{
	m_work_thread.put_event_execution_request(
		agent_ref,
		event_count );
}

} /* namespace impl */

} /* namespace one_thread */

} /* namespace disp */

} /* namespace so_5 */