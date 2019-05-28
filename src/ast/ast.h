#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <ostream>

#include "data/gc.h"
#include "data/stats.h"

/// Base class for all AST objects
class ASTNode : public GC_Object {
public:

#ifdef RP_STATS
	ASTNode() { ++allocs_in[crnt_pass]; }
#endif

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
