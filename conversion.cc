#include <algorithm>
#include <ostream>

#include "conversion.h"
#include "data.h"
#include "gc.h"

const ConversionGraph::ConversionList ConversionGraph::no_conversions = {};

GC& operator<< (const GC& gc, const ConversionGraph& g) {
	for ( const auto& v : g.nodes ) {
		gc << v.first << v.second.type;
	}
}

std::ostream& operator<< ( std::ostream& out, const ConversionGraph& g ) {
	for ( const auto& u : g.nodes ) {
		out << *u.first << std::endl;
		for ( const auto& v : u.second.conversions ) {
			out << " => " << *(v.to->type) << " " << v.cost << std::endl; 
		}
	}
	
	return out;
}

ConversionGraph make_conversions( CanonicalTypeMap& types ) {
	ConversionGraph g;

#ifdef RP_USER_CONVS
	#error User conversions not yet implemented
#else // ! RP_USER_CONVS
	// loop over "from" types
	for ( auto from = types.begin(); from != types.end(); ++from ) {
		const ConcType* f = as_safe<ConcType>(from->second);
		if ( ! f ) continue;
		ConversionGraph::ConversionNode& fromNode = g.try_insert( f );

		// loop over "to" types
		auto to = from;
		for (++to; to != types.end(); ++to) {
			const ConcType* t = as_safe<ConcType>(to->second);
			if ( ! t ) continue;
			ConversionGraph::ConversionNode& toNode = g.try_insert( t );
			
			fromNode.conversions.emplace_back( &toNode, Cost::from_diff( t->id() - f->id() ) );
			toNode.conversions.emplace_back( &fromNode, Cost::from_diff( f->id() - t->id() ) );
		}

		// done with from type, ensure conversion list sorted
		std::sort( fromNode.conversions.begin(), fromNode.conversions.end(),
		           []( const Conversion& a, const Conversion& b ) {
					   return a.cost < b.cost;
				   });
	}
#endif
	
	return g;
}
