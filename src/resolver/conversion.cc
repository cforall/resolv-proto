#include <algorithm>
#include <ostream>

#include "conversion.h"

#include "data/cast.h"
#include "data/gc.h"

const GC& operator<< (const GC& gc, const ConversionGraph& g) {
	for ( const auto& v : g.nodes ) {
		gc << v.second.type;
	}
	return gc;
}

std::ostream& operator<< ( std::ostream& out, const ConversionGraph& g ) {
	for ( const Conversion& c : g.edges ) {
		out << *(c.from->type) << " => " << *(c.to->type) << ' ' << c.cost << std::endl;
	}
	
	return out;
}

ConversionGraph make_conversions( CanonicalTypeMap& types ) {
	using ConversionRef = ConversionGraph::ConversionRef;
	using ConversionNode = ConversionGraph::ConversionNode;

	ConversionGraph g;

#ifdef RP_USER_CONVS
	#error User conversions not yet implemented
#else // ! RP_USER_CONVS
	// loop over "from" types
	for ( auto from = types.begin(); from != types.end(); ++from ) {
		const ConcType* f = as_safe<ConcType>(from->second);
		if ( ! f ) continue;
		ConversionNode& fromNode = g.try_insert( f );

		// loop over "to" types
		auto to = from;
		for (++to; to != types.end(); ++to) {
			const ConcType* t = as_safe<ConcType>(to->second);
			if ( ! t ) continue;
			ConversionNode& toNode = g.try_insert( t );

			// make conversions
			ConversionRef out = g.edges.size(), in = g.edges.size() + 1;
			g.edges.emplace_back( &toNode, &fromNode, Cost::from_diff( t->id() - f->id() ) );
			g.edges.emplace_back( &fromNode, &toNode, Cost::from_diff( f->id() - t->id() ) );

			// add to nodes
			fromNode.out.push_back( out );
			fromNode.in.push_back( in );
			toNode.out.push_back( in );
			toNode.in.push_back( out );
		}

		// done with from type, ensure conversion lists are sorted
		auto conversionListSort = [&g]( ConversionRef a, ConversionRef b )  {
			return g.edges[a].cost < g.edges[b].cost;
		};
		std::sort( fromNode.out.begin(), fromNode.out.end(), conversionListSort );
		std::sort( fromNode.in.begin(), fromNode.in.end(), conversionListSort );
	}
#endif
	
	return g;
}
