/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Error handling flags definition.
*/

#if !defined( _SO_5__THROWING_STRATEGY_HPP_ )
#define _SO_5__THROWING_STRATEGY_HPP_

namespace so_5
{

//
// throwing_strategy_t
//

//! Flags to indicate desired action in case of errors inside
//! some of SObjectizer functions/methods.
enum throwing_strategy_t
{
	//! An exception should be thrown in case of error.
	THROW_ON_ERROR,
	//! Do not throw exception but return error code instead.
	DO_NOT_THROW_ON_ERROR
};

} /* namespace so_5  */

#endif
