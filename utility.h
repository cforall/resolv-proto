#pragma once

#include <memory>
#include <utility>

/// Make an expression pointer; T must be a subclass of U
template<typename U, typename T, typename... Args>
inline std::unique_ptr<U> ptr( Args&&... args ) {
	return std::unique_ptr<U>{ 
		dynamic_cast<U*>( new T( std::forward<Args>(args)... ) ) };
}