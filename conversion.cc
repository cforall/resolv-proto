#include "conversion.h"
#include "utility.h"

ConversionGraph::ConversionList ConversionGraph::no_conversions = {};

ConversionGraph make_conversions( const SortedSet<ConcType>& types ) {
	ConversionGraph g;
	
	// loop over from types
	for ( auto from = types.begin(); from != types.end(); ++from ) {
		// do safe conversions
		auto to = from;
		++to;
		while ( to != types.end() ) {
			Ref<ConcType> f = ref( *from );
			Ref<ConcType> t = ref( *to );
			g.insert( f, t, Cost{ 0, t->id() - f->id() } );
			
			++to;
		}
		
		// do unsafe conversions
		if ( from == types.begin() ) continue; // --types.begin() is invalid
		to = from;
		do {
			--to;
			
			Ref<ConcType> f = ref( *from );
			Ref<ConcType> t = ref( *to );
			g.insert( f, t, Cost{ f->id() - t->id(), 0 } );
		} while ( to != types.begin() );
	}
	
	return g;
}
