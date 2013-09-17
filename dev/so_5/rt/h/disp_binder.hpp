/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Interface for dispatcher binders definition.
*/

#if !defined( _SO_5__RT__DISP_BINDER_HPP_ )
#define _SO_5__RT__DISP_BINDER_HPP_

#include <memory>

#include <so_5/h/declspec.hpp>
#include <so_5/rt/h/agent_ref.hpp>
#include <so_5/rt/h/disp.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

class so_environment_impl_t;

} /* namespace impl */


class so_environment_t;

//
// disp_binder_t
//

//! Interface for dispatcher binders.
/*!
 * Dispatcher binders are used in agent registration process to
 * binding of agents to desired dispatchers.
 */
class SO_5_TYPE disp_binder_t
{
	public:
		disp_binder_t();
		virtual ~disp_binder_t();

		//! Bind agent to dispatcher.
		virtual void
		bind_agent(
			//! SObjectizer Environment where agent/cooperation/dispatcher
			//! are working.
			impl::so_environment_impl_t & env,
			//! Agent to be bound.
			agent_ref_t & agent_ref ) = 0;

		//! Unbind agent from dispatcher.
		virtual void
		unbind_agent(
			//! SObjectizer Environment where agent/cooperation/dispatcher
			//! are working.
			impl::so_environment_impl_t & env,
			//! Agent to be bound.
			agent_ref_t & agent_ref ) = 0;
};

//! Typedef for disp_binder autopointer.
typedef std::unique_ptr< disp_binder_t > disp_binder_unique_ptr_t;

//! Typedef for disp_binder smart pointer.
typedef std::shared_ptr< disp_binder_t >
	disp_binder_ref_t;

//! Create an instance of default dispatcher binding.
SO_5_EXPORT_FUNC_SPEC( disp_binder_unique_ptr_t )
create_default_disp_binder();

} /* namespace rt */

} /* namespace so_5 */

#endif