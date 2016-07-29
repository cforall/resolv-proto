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
	
	// no conversions for empty or single-element set
	if ( types.size() <= 1 ) return g;

	auto high = types.begin();
	auto low = high;
	++high;
	do {
		Brw<ConcType> lo = brw( *low );
		Brw<ConcType> hi = brw( *high );

		// safe conversion
		g.insert( lo, hi, Cost::from_diff( hi->id() - lo->id() ) );
		// unsafe conversion
		g.insert( hi, lo, Cost::from_diff( lo->id() - hi->id() ) );

		low = high;
		++high;
	} while ( high != types.end() );

	return g;
}
