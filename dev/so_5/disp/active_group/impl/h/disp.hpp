/*
	SObjectizer 5.
*/

/*!
	\file
	\brief The real class of active group dispatcher
*/

#if !defined( _SO_5__DISP__ACTIVE_GROUP__IMPL__DISP_HPP_ )
#define _SO_5__DISP__ACTIVE_GROUP__IMPL__DISP_HPP_

#include <map>

#include <ace/Thread_Mutex.h>

#include <so_5/rt/h/disp.hpp>

namespace so_5 {

namespace rt
{

class agent_t;

} /* namespace rt */


namespace disp {

namespace active_group {

namespace impl {

//
// dispatcher_t
//

/*!
	\brief Active group dispatcher.
*/
class dispatcher_t
	:
		public so_5::rt::dispatcher_t
{
	public:
		dispatcher_t();
		virtual ~dispatcher_t();

		//! \name Implementation of so_5::rt::dispatcher methods.
		//! \{

		//! Launch the dispatcher.
		virtual ret_code_t
		start();

		//! Send a signal about shutdown to the dispatcher.
		virtual void
		shutdown();

		//! Wait for the full stop of the dispatcher.
		virtual void
		wait();

		/*!
		 * \brief Schedule event processing for the agent.
		 *
		 * \note Should not be called directly for this dispatcher.
		 */
		virtual void
		put_event_execution_request(
			const so_5::rt::agent_ref_t &,
			unsigned int);
		//! \}

		/*!
		 * \brief Get the dispatcher for specified active group.
		 *
		 * If name \a group_name is unknown then a new dispatcher
		 * thread is started. This thread is marked as it has one
		 * working agent on it.
		 *
		 * If there already is a thread for \a group_name then the
		 * counter of working agents for it is incremented.
		 */
		so_5::rt::dispatcher_t &
		query_disp_for_group( const std::string & group_name );

		/*!
		 * \brief Release the dispatcher for specified active group.
		 *
		 * Decrements working agent count for the thread of
		 * \a group_name. If there no more working agents left then
		 * dispatcher and working thread for that group will be
		 * destroyed.
		 */
		void
		release_disp_for_group( const std::string & group_name );

	private:
		//! Auxiliary class for working agent counting.
		struct disp_with_ref_t
		{
			disp_with_ref_t()
				:
					m_user_agent( 0 )
			{}

			disp_with_ref_t(
				const so_5::rt::dispatcher_ref_t & disp_ref,
				unsigned int user_agent )
				:
					m_disp_ref( disp_ref ),
					m_user_agent( user_agent)
			{}

			so_5::rt::dispatcher_ref_t m_disp_ref;
			unsigned int m_user_agent;
		};

		//! Typedef for mapping from group names to single thread
		//! dispatcher.
		typedef std::map<
				std::string,
				disp_with_ref_t >
			active_group_disp_map_t;

		//! A map of dispatchers for active groups.
		active_group_disp_map_t m_group_disp;

		//! Shutdown indication flag.
		bool m_shutdown_started;

		//! This object lock.
		ACE_Thread_Mutex m_lock;
};

} /* namespace impl */

} /* namespace active_group */

} /* namespace disp */

} /* namespace so_5 */

#endif