#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <ostream>

#include "data/gc.h"

/// Base class for all AST objects
class ASTNode : public GC_Object {
public:
	/// Printing modes
	enum class Print {
		Default,     ///< Using the default style.
		InputStyle,  ///< Styled to emulate the input format.
		Concise      ///< Concise printing style
	};

	/// Print this node, using the named style
	virtual void write( std::ostream& out, Print style = Print::Default ) const = 0;
};

inline std::ostream& operator<< (std::ostream& out, const ASTNode& n) {
	n.write(out);
	return out;
};
