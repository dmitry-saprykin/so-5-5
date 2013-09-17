/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A base class for agents.
*/

#if !defined( _SO_5__RT__AGENT_HPP_ )
#define _SO_5__RT__AGENT_HPP_

#include <memory>
#include <map>

#include <so_5/h/declspec.hpp>
#include <so_5/h/types.hpp>

#include <so_5/h/ret_code.hpp>
#include <so_5/h/throwing_strategy.hpp>

#include <so_5/rt/h/atomic_refcounted.hpp>
#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/agent_ref.hpp>
#include <so_5/rt/h/subscription_bind.hpp>
#include <so_5/rt/h/subscription_key.hpp>
#include <so_5/rt/h/event_caller_block_ref.hpp>
#include <so_5/rt/h/agent_state_listener.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

// Forward declarations.
class local_event_queue_t;
class message_consumer_link_t;
class so_environment_impl_t;
class state_listener_controller_t;

} /* namespace impl */

class state_t;
class so_environment_t;
class agent_coop_t;

//
// agent_t
//

//! A base class for agents.
/*!
	An agent in SObjctizer should be derived from agent_t.

	The base class provides various methods whose could be splitted into
	the following groups:
	\li methods for interaction with SObjectizer;
	\li predefined hook-methods which are called during: cooperation
	registration, starting and stopping of agent;
	\li methods for message subscription and unsubscription;
	\li methods for working with agent state;

	<b>Methods for interaction with SObjectizer</b>

	Method so_5::rt::agent_t::so_environment() serves for access to
	SObjectizer Environment (and, therefore, to all methods of
	SObjectizer Environment).
	This method could be called immediatelly after agent creation.
	This is because agent is bound to SObjectizer Environment during
	creation process.

	<b>Hook methods</b>

	Base class defines several hook-methods. Its default implementation
	do nothing.

	Method agent_t::so_define_agent() is called just before agent will
	started by SObjectizer as a part of agent registration process.
	It should be reimplemented for initial subscription of agent
	to messages.

	There are two hook-methods related to important agent's life-time events:
	agent_t::so_evt_start() and agent_t::so_evt_finish(). They are called
	by SObjectizer in next circumstances:
	- method so_evt_start() is called when agent is starting its work
	  inside SObjectizer. At that moment the agent cooperation is
	  successfully registered and all agent are bound to their working
	  threads;
	- method so_evt_finish() is called during the agent's cooperation
	  deregistration just after agent processed the last pending event.

	Methods so_evt_start() and so_evt_finish() are called by SObjectizer and
	user can just reimplement them to implement agent-specific logic.

	<b>Message subscription and unsubscription methods</b>

	Any method with the following prototype can be used as event
	handler:
	\code
		void
		evt_handler(
			const so_5::rt::event_data_t< MESSAGE > & msg );
	\endcode
	Where \c evt_handler is a name of event handler, \c MESSAGE is a message
	type.

	Class so_5::rt::event_data_t is a wrapper on pointer to an instance of \c
	MESSAGE. It is very similar to <tt>std::unique_ptr</tt>. The pointer to \c
	MESSAGE could be nullptr. It happens in case when message has no actual data
	and servers just a signal about something.

	Subscription to a message is performed by method so_subscribe().
	This method returns an instance of so_5::rt::subscription_bind_t which
	does all actual actions of subscription process. That instance already
	knowns agents and message mbox and uses default agent state for
	the event subscription (binding to different state is also possible). 

	<b>Methods for working with agent state</b>

	Agent can change its state by so_change_state() method.

	An attempt to switch agent to the state which belongs to different
	agent is an error. If state is belong to the same agent there are
	no possibility to any run-time errors. In this case changing agent
	state is very safe operation.

	In some cases it is necessary to detect agent state switching.
	For example for application monitoring purposes. That could be done
	by "state listeners".

	Any count of state listeners could be set for an agent. There are
	two methods for that:
	- so_add_nondestroyable_listener() is for listeners whose life-time
	  are controlled by programmer, not by SObjectizer;
	- so_add_destroyable_listener() is for listeners whose life-time
	  should be controlled by SObjectizer. For those listeners agent tooks
	  care about their deleting.
*/
class SO_5_TYPE agent_t
	:
		private atomic_refcounted_t
{
		friend class subscription_bind_t;
		friend class subscription_unbind_t;
		friend class agent_ref_t;
		friend class agent_coop_t;

	public:
		//! Constructor.
		/*!
			Agent should be bound to SObjectizer Environment during
			its creation. And that binding cannot be changed anymore.
		*/
		explicit agent_t(
			//! The Environment for that agent is created.
			so_environment_t & env );

		virtual ~agent_t();

		//! Get raw pointer to itself.
		/*!
			This method is intended for use in member initialization
			list instead 'this' to suppres compiler warnings.
			For example for agent state initialization:
			\code
			class a_sample_t
				:
					public so_5::rt::agent_t
			{
					typedef so_5::rt::agent_t base_type_t;

					// Agent state.
					const so_5::rt::state_t m_sample_state;
				public:
					a_sample_t( so_5::rt::so_environment_t & env )
						:
							base_type_t( env ),
							m_sample_state( self_ptr() )
					{
						// ...
					}

				// ...

			};
			\endcode
		*/
		const agent_t *
		self_ptr() const;

		//! Hook on agent start inside SObjectizer.
		/*!
			It is guaranteed that this method will be called first
			just after end of cooperation registration process.

			During cooperation registration agent is bound to some
			working thread. And the first method which is called for
			the agent on that working thread context is that method.

			\code
			class a_sample_t
				:
					public so_5::rt::agent_t
			{
				// ...
				virtual void
				so_evt_start();
				// ...
			};

			a_sample_t::so_evt_start()
			{
				std::cout << "first agent action on bound dispatcher" << std::endl;
				... // Some application logic actions.
			}
			\endcode
		*/
		virtual void
		so_evt_start();

		//! Hook of agent finish in SObjectizer.
		/*!
			It is guaranteed that this method will be called last
			just before deattaching agent from it's working thread.

			This method could be used to perform some cleanup
			actions on it's working thread.
			\code
			class a_sample_t
				:
					public so_5::rt::agent_t
			{
				// ...
				virtual void
				so_evt_finish();
				// ...
			};

			a_sample_t::so_evt_finish()
			{
				std::cout << "last agent activity";

				if( so_current_state() == m_db_error_happened )
				{
					// Delete DB connection on the same thread where
					// connection was established and where some
					// error happened.
					m_db.release();
				}
			}
			\endcode
		*/
		virtual void
		so_evt_finish();

		//! Access to the current agent state.
		inline const state_t &
		so_current_state() const
		{
			return *m_current_state_ptr;
		}

		//! Name of agent's cooperation.
		/*!
		 * \note It is safe to use this method when agent is working inside
		 * SObjectizer because agent could be registered only in some
		 * cooperation. When agent is not registered in SObjectizer this
		 * method should be used with care.
		 *
		 * \throw so_5::exception_t If agent doesn't belong to any cooperation.
		 *
		 * \return Name of cooperation if agent is bound to a cooperation.
		 */
		const std::string &
		so_coop_name() const;

		//! Add state listener to agent.
		/*!
		 * A programmer should guaranteed that life-time of
		 * \a state_listener is exceed life-time of the agent.
		 */
		void
		so_add_nondestroyable_listener(
			agent_state_listener_t & state_listener );

		//! Add state listener to agent.
		/*!
		 * Agent tooks care of destruction of \a state_listener.
		 */
		void
		so_add_destroyable_listener(
			agent_state_listener_unique_ptr_t state_listener );

		//! Push an event to agent's event queue.
		/*!
			This method is used by SObjectizer for agent's event scheduling.
		*/
		static inline void
		call_push_event(
			agent_t & agent,
			const event_caller_block_ref_t & event_handler_caller,
			const message_ref_t & message )
		{
			agent.push_event( event_handler_caller, message );
		}

		//! Run event handler for next event.
		/*!
			This method is used by dispatcher/working thread for
			event handler execution.
		*/
		static inline void
		call_next_event(
			//! Agents which events should be executed.
			agent_t & agent )
		{
			agent.exec_next_event();
		}

		//! Bind agent to the dispatcher.
		static inline void
		call_bind_to_disp(
			agent_t & agent,
			dispatcher_t & disp )
		{
			agent.bind_to_disp( disp );
		}

	protected:
		/*!
		 * \name Methods for working with agent state.
		 * \{
		 */

		//! Access to the agent's default state.
		const state_t &
		so_default_state() const;

		//! Change state.
		/*!
			Usage sample:
			\code
			void
			a_sample_t::evt_smth(
				const so_5::rt::event_data_t< message_one_t > & msg )
			{
				// If something wrong with message then we should
				// switch to error_state.
				if( error_in_data( *msg ) )
					so_change_state( m_error_state );
			}
			\endcode
		*/
		ret_code_t
		so_change_state(
			//! New agent state.
			const state_t & new_state,
			//! Exception strategy.
			throwing_strategy_t throwing_strategy = THROW_ON_ERROR );
		/*!
		 * \}
		 */

		/*!
		 * \name Subscription and unsubscription methods.
		 * \{
		 */

		//! Initiate subscription.
		/*!
			Usage sample:
			\code
			void
			a_sample_t::so_define_agent()
			{
				so_subscribe( m_mbox_target )
					.in( m_state_one )
						.event( &a_sample_t::evt_sample_handler );
			}
			\endcode
		*/
		subscription_bind_t
		so_subscribe(
			//! Mbox for messages to subscribe.
			const mbox_ref_t & mbox_ref );

		//! Initiate unsubscription.
		/*!
			Usage sample:
			\code
			void
			a_sample_t::evt_smth(
				const so_5::rt::event_data_t< message_one_t > & msg )
			{
				so_unsubscribe( m_mbox_target )
					.in( m_state_one )
						.event( &a_sample_t::evt_sample_handler );
			}
			\endcode
		*/
		subscription_unbind_t
		so_unsubscribe(
			//! Mbox for messages to unsubscribe.
			const mbox_ref_t & mbox_ref );
		/*!
		 * \}
		 */

		/*!
		 * \name Agent initialization methods.
		 * \{
		 */

		//! Hook on define agent for SObjectizer.
		/*!
			This method is called by SObjectizer during cooperation
			registration process before agent will be bound to its
			working thread.

			May be used by agent to make necessary message subscriptions.

			Usage sample;
			\code
			class a_sample_t
				:
					public so_5::rt::agent_t
			{
				// ...
				virtual void
				so_define_agent();

				void
				evt_handler_1(
					const so_5::rt::event_data_t< message1_t > & msg );
				// ...
				void
				evt_handler_N(
					const so_5::rt::event_data_t< messageN_t > & msg );

			};

			void
			a_sample_t::so_define_agent()
			{
				// Make subscriptions...
				so_subscribe( m_mbox1 )
					.in( m_state_1 )
						.event( &a_sample_t::evt_handler_1 );
				// ...
				so_subscribe( m_mboxN )
					.in( m_state_N )
						.event( &a_sample_t::evt_handler_N );
			}
			\endcode
		*/
		virtual void
		so_define_agent();

		//! Is method define_agent already called?
		/*!
			Usage sample:
			\code
			class a_sample_t
				:
					public so_5::rt::agent_t
			{
				// ...

				public:
					void
					set_target_mbox(
						const so_5::rt::mbox_ref_t & mbox )
					{
						// mbox cannot be changed after agent registration.
						if( !so_was_defined() )
						{
							m_target_mbox = mbox;
						}
					}

				private:
					so_5::rt::mbox_ref_t m_target_mbox;
			};
			\endcode
		*/
		bool
		so_was_defined() const;
		/*!
		 * \}
		 */

	public:
		//! Access to SObjectizer Environment for that agent is belong.
		/*!
			Usage sample for other cooperation registration:
			\code
			void
			a_sample_t::evt_on_smth(
				const so_5::rt::event_data_t< some_message_t > & msg )
			{
				so_5::rt::agent_coop_unique_ptr_t coop =
					so_environment().create_coop(
						so_5::rt::nonempty_name_t( "first_coop" ) );

				// Filling the cooperation...
				coop->add_agent( so_5::rt::agent_ref_t(
					new a_another_t( ... ) ) );
				// ...

				// Registering cooperation.
				so_environment().register_coop( coop );
			}
			\endcode

			Usage sample for SObjectizer shutting down:
			\code
			void
			a_sample_t::evt_last_event(
				const so_5::rt::event_data_t< message_one_t > & msg )
			{
				...
				so_environment().stop();
			}
			\endcode
		*/
		so_environment_t &
		so_environment();

	private:
		//! Make agent reference.
		/*!
		 * This is an internal SObjectizer method. It is called when
		 * it is guaranteed that agent is still necessary and something
		 * has reference to it.
		 */
		agent_ref_t
		create_ref();

		/*!
		 * \name Embedding agent into SObjectize run-time.
		 * \{
		 */

		//! Bind agent to cooperation.
		/*!
		 * Initializes internal cooperation pointer.
		 *
		 * Drops m_is_coop_deregistered to false.
		 */
		void
		bind_to_coop(
			//! Cooperation for that agent.
			agent_coop_t & coop );

		//! Bind agent to SObjectizer Environment.
		/*!
		 * Called from agent constructor.
		 *
		 * Initializes internal SObjectizer Environment pointer.
		 */
		void
		bind_to_environment(
			impl::so_environment_impl_t & env_impl );

		//! Bind agent to dispatcher.
		/*!
		 * Initializes internal dispatcher poiner.
		 *
		 * Checks local event queue. If queue is not empty then
		 * tells dispatcher to schedule agent for event processing.
		 */
		void
		bind_to_disp(
			dispatcher_t & disp );

		//! Agent definition driver.
		/*!
		 * Calls so_define_agent() and then stores agent definition flag.
		 */
		void
		define_agent();

		//! Agent undefinition deriver.
		/*!
		 * Destroys all agent subscriptions.
		*/
		void
		undefine_agent();
		/*!
		 * \}
		 */

		/*!
		 * \name Subscription/unsubscription implementation details.
		 * \{
		 */

		//! Create binding between agent and mbox.
		ret_code_t
		create_event_subscription(
			//! Message type.
			const type_wrapper_t & type_wrapper,
			//! Message's mbox.
			mbox_ref_t & mbox_ref,
			//! Event handler caller.
			const event_handler_caller_ref_t & ehc,
			//! Exception strategy.
			throwing_strategy_t throwing_strategy );

		//! Destroy agent and mbox binding.
		ret_code_t
		destroy_event_subscription(
			//! Message type.
			const type_wrapper_t & type_wrapper,
			//! Message's mbox.
			mbox_ref_t & mbox_ref,
			//! Event handler caller.
			const event_handler_caller_ref_t & ehc,
			//! Exception strategy.
			throwing_strategy_t throwing_strategy );

		//! Destroy all agent subscriptions.
		void
		destroy_all_subscriptions();
		/*!
		 * \}
		 */

		/*!
		 * \name Event handling implementation details.
		 * \{
		 */

		//! Push event into local event queue.
		void
		push_event(
			//! Event handler caller for event.
			const event_caller_block_ref_t & event_handler_caller,
			//! Event message.
			const message_ref_t & message );

		//! Execute next event.
		/*!
		 * \attention Should be called only on working thread contex.
		 *
		 * \attention It is assumed that local event queue is not empty.
		 */
		void
		exec_next_event();
		/*!
		 * \}
		 */

		//! Default agent state.
		const state_t m_default_state;

		//! Current agent state.
		const state_t * m_current_state_ptr;

		//! Agent definition flag.
		/*!
		 * Set to true after successful return from so_define_agent().
		 */
		bool m_was_defined;

		//! State listeners controller.
		std::unique_ptr< impl::state_listener_controller_t >
			m_state_listener_controller;

		//! Typedef for map from subscriptions to event handlers.
		typedef std::map<
				subscription_key_t,
				impl::message_consumer_link_t * >
			consumers_map_t;

		//! Map from subscriptions to event handlers.
		consumers_map_t m_event_consumers_map;

		//! Local events queue.
		std::unique_ptr< impl::local_event_queue_t >
			m_local_event_queue;

		//! SObjectizer Environment for which the agent is belong.
		impl::so_environment_impl_t * m_so_environment_impl;

		//! Dispatcher for that agent.
		/*!
		 * By default this pointer points to a special stub.
		 * This stub do nothing but allows to safely call method for
		 * events scheduling.
		 *
		 * This pointer received actual value after binding
		 * agent to real dispatcher.
		 */
		dispatcher_t * m_dispatcher;

		//! Cooperation to which agent is belong.
		agent_coop_t * m_agent_coop;

		//! Is cooperation deregistration in progress?
		bool m_is_coop_deregistered;
};

} /* namespace rt */

} /* namespace so_5 */

#endif
