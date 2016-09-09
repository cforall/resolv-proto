#include <ostream>

#include "conversion.h"
#include "gc.h"

const ConversionGraph::ConversionList ConversionGraph::no_conversions = {};

GC& operator<< (const GC& gc, const ConversionGraph& g) {
	for ( auto& v : g.nodes ) {
		gc << v.first << v.second.type;
	}
}

std::ostream& operator<< ( std::ostream& out, const ConversionGraph& g ) {
	for ( auto& u : g.nodes ) {
		out << *u.first << std::endl;
		for ( auto& v : u.second.conversions ) {
			out << " => " << *(v.to->type) << " " << v.cost << std::endl; 
		}
	}
	
	return out;
}

ConversionGraph make_conversions( SortedSet<ConcType>& types ) {
	ConversionGraph g;

	// no conversions for empty or single-element set
	if ( types.size() <= 1 ) return g;
	
#ifdef RP_USER_CONVS
	// generate single-step conversions between all types, adding more if needed
	int min = (*types.begin())->id();
	int max = (*types.rbegin())->id();

	// safe conversions first
	const ConcType* prev = *types.begin();
	for (int i = min+1; i < max; ++i) {
		// insert type if needed
		const ConcType* crnt = get_canon( types, i );
		// single-step safe conversion
		g.insert( prev, crnt, Cost{ 0, 1 } );

		prev = crnt;
	}
	const ConcType* crnt = *types.rbegin();
	g.insert( prev, crnt, Cost{ 0, 1 } );

	// then unsafe
	prev = crnt;
	auto j = types.rbegin();
	for (++j; j != types.rend(); ++j) {
		crnt = *j;  // inserted by prior loop
		// single-step unsafe conversion
		g.insert( prev, crnt, Cost{ 1, 0 } );

		prev = crnt;
	}
#else // ! RP_USER_CONVS
	// loop over from types
	for ( auto from = types.begin(); from != types.end(); ++from ) {
		// do safe conversions
		auto to = from;
		++to;
		while ( to != types.end() ) {
			const ConcType* f = *from;
			const ConcType* t = *to;
			g.insert( f, t, Cost::from_diff( t->id() - f->id() ) );
			
			++to;
		}
		
		// do unsafe conversions
		if ( from == types.begin() ) continue; // --types.begin() is invalid
		to = from;
		do {
			--to;
			
			const ConcType* f = *from;
			const ConcType* t = *to;
			g.insert( f, t, Cost::from_diff( t->id() - f->id() ) );
		} while ( to != types.begin() );
	}
#endif
	
	return g;
}
