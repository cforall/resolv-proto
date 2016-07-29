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

ConversionGraph make_conversions( SortedSet<ConcType>& types ) {
	ConversionGraph g;

	// no conversions for empty or single-element set
	if ( types.size() <= 1 ) return g;
	
#ifdef RP_USER_CONVS
	// generate single-step conversions between all types, adding more if needed
	int min = (*types.begin())->id();
	int max = (*types.rbegin())->id();

	// safe conversions first
	Brw<ConcType> prev = brw( *types.begin() );
	for (int i = min+1; i < max; ++i) {
		// insert type if needed
		Brw<ConcType> crnt = get_canon( types, make_ptr<ConcType>( i ) );
		// single-step safe conversion
		g.insert( prev, crnt, Cost{ 0, 1 } );

		prev = crnt;
	}
	Brw<ConcType> crnt = brw( *types.rbegin() );
	g.insert( prev, crnt, Cost{ 0, 1 } );

	// then unsafe
	prev = crnt;
	auto j = types.rbegin();
	for (++j; j != types.rend(); ++j) {
		crnt = brw( *j );  // inserted by prior loop
		// single-step unsafe conversion]
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
#endif
	
	return g;
}
