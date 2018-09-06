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

struct ConversionListCompare {
	const ConversionGraph& g;

	ConversionListCompare(const ConversionGraph& g) : g{g} {}

	bool operator() ( ConversionGraph::ConversionRef a, ConversionGraph::ConversionRef b ) {
		return g.edges[a].cost < g.edges[b].cost;
	}
};

ConversionGraph make_conversions( CanonicalTypeMap& types ) {
	using ConversionRef = ConversionGraph::ConversionRef;
	using ConversionNode = ConversionGraph::ConversionNode;

	ConversionGraph g;
	auto conversionListSort = ConversionListCompare{g};

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
		std::sort( fromNode.out.begin(), fromNode.out.end(), conversionListSort );
		std::sort( fromNode.in.begin(), fromNode.in.end(), conversionListSort );
	}
#endif
	
	return g;
}

/// Inserts a ref into a list in its properly sorted position
void sortedInsert( ConversionGraph::ConversionList& l, ConversionGraph::ConversionRef x, 
		ConversionListCompare cmp ) {
	l.insert( std::upper_bound( l.begin(), l.end(), x, cmp ) , x );
}

void ConversionGraph::addType( const Type* from ) {
	auto conversionListSort = ConversionListCompare{*this};

	// find "from" type, skipping if not concrete or already present
	const ConcType* f = as_safe<ConcType>( from );
	if ( ! f ) return;
	if ( nodes.count( f ) ) return;

	// insert "from" type into the node set
	ConversionNode& fromNode = nodes.insert( f, ConversionNode{ from } ).first->second;

	// loop over "to" types
	for ( auto& toEntry : nodes ) {
		// find "to" node, skipping if not concrete or the "from" type
		const ConcType* t = as_safe<ConcType>(toEntry.first);
		if ( ! t ) continue;
		ConversionNode& toNode = toEntry.second;
		if ( &toNode == &fromNode ) continue;

		// make conversions
		ConversionRef out = edges.size(), in = edges.size() + 1;
		edges.emplace_back( &toNode, &fromNode, Cost::from_diff( t->id() - f->id() ) );
		edges.emplace_back( &fromNode, &toNode, Cost::from_diff( f->id() - t->id() ) );

		// add to nodes
		fromNode.out.push_back( out );
		fromNode.in.push_back( in );
		sortedInsert( toNode.out, in, conversionListSort );
		sortedInsert( fromNode.in, out, conversionListSort );
	}

	// done with new type, ensure conversion lists are sorted
	std::sort( fromNode.out.begin(), fromNode.out.end(), conversionListSort );
	std::sort( fromNode.in.begin(), fromNode.in.end(), conversionListSort );
}
