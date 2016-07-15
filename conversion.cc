#include <ostream>

#include "conversion.h"
#include "utility.h"

const ConversionGraph::ConversionList ConversionGraph::no_conversions = {};

std::ostream& operator<< ( std::ostream& out, const ConversionGraph& g ) {
	for ( auto& u : g.nodes ) {
		out << *u.first << std::endl;
		for ( auto& v : u.second.conversions ) {
			out << " => " << *(v.to->type) << " " << v.cost << std::endl; 
		}
	}
	
	return out;
}

ConversionGraph make_conversions( const SortedSet<ConcType>& types ) {
	ConversionGraph g;
	
	// loop over from types
	for ( auto from = types.begin(); from != types.end(); ++from ) {
		// do safe conversions
		auto to = from;
		++to;
		while ( to != types.end() ) {
			Brw<ConcType> f = brw( *from );
			Brw<ConcType> t = brw( *to );
			g.insert( f, t, Cost::from_diff( t->id() - f->id() ) );
			
			++to;
		}
		
		// do unsafe conversions
		if ( from == types.begin() ) continue; // --types.begin() is invalid
		to = from;
		do {
			--to;
			
			Brw<ConcType> f = brw( *from );
			Brw<ConcType> t = brw( *to );
			g.insert( f, t, Cost::from_diff( t->id() - f->id() ) );
		} while ( to != types.begin() );
	}
	
	return g;
}
