/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Agent to dispatcher binder interface.
*/

#if !defined( _SO_5__DISP__ACTIVE_GROUP__IMPL__DISP_BINDER_HPP_ )
#define _SO_5__DISP__ACTIVE_GROUP__IMPL__DISP_BINDER_HPP_

#include <so_5/rt/h/disp_binder.hpp>
#include <so_5/rt/h/agent_ref.hpp>
#include <so_5/rt/impl/h/so_environment_impl.hpp>

namespace so_5
{

namespace disp
{

namespace active_group
{

namespace impl
{

//! Agent to dispatcher binder interface.
class disp_binder_t
	:
		public so_5::rt::disp_binder_t
{
	public:
		disp_binder_t(
			const std::string & disp_name,
			const std::string & group_name );

		virtual ~disp_binder_t();

		//! Bind agent to dispatcher.
		virtual void
		bind_agent(
			so_5::rt::impl::so_environment_impl_t & env,
			so_5::rt::agent_ref_t & agent_ref );

		//! Unbind agent from dispatcher.
		virtual void
		unbind_agent(
			so_5::rt::impl::so_environment_impl_t & env,
			so_5::rt::agent_ref_t & agent_ref );

	private:
		//! Name of dispatcher to be bound to.
		const std::string m_disp_name;

		//! Name of active group to be included in.
		const std::string m_group_name;
};

} /* namespace impl */

} /* namespace active_group */

} /* namespace disp */

} /* namespace so_5 */

#endif