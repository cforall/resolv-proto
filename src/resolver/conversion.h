#pragma once

#include <iterator>
#include <ostream>
#include <utility>
#include <vector>

#include "canonical_type_map.h"
#include "cost.h"
#include "type_map.h"

#include "ast/type.h"
#include "data/gc.h"
#include "data/list.h"

/// Graph of conversions
class ConversionGraph {
	friend const GC& operator<< (const GC&, const ConversionGraph&);
	friend std::ostream& operator<< (std::ostream&, const ConversionGraph&);
	friend ConversionGraph make_conversions( CanonicalTypeMap& types );
public:
	struct ConversionNode;

	/// Reference to a conversion in a root conversion graph
	using ConversionRef = unsigned;

	/// List of ConversionRef
	using ConversionList = std::vector<ConversionRef>;
	
	/// Edge in the conversion graph representing a direct conversion
	struct Conversion {
		const ConversionNode* to;    ///< Type converted to
		const ConversionNode* from;  ///< Type converted from
		Cost cost;                   ///< Cost of this conversion
		
		Conversion() = default;
		Conversion(const ConversionNode* to, const ConversionNode* from, Cost cost) 
			: to(to), from(from), cost(cost) {}
		
		Conversion(const Conversion&) = default;
		Conversion(Conversion&&) = default;
		Conversion& operator= (const Conversion&) = default;
		Conversion& operator= (Conversion&&) = default;

		bool operator== (const Conversion& o) { return to == o.to && from == o.from; }
		bool operator!= (const Conversion& that) { return !(*this == that); }
	};
	
	/// Node in the conversion graph representing one type
	struct ConversionNode {
		const Type* type;    ///< Type to convert from
		ConversionList out;  ///< Indices of conversions from this type
		ConversionList in;   ///< Indices of conversions to this type
		
		ConversionNode() = default;
		ConversionNode(const Type* type) : type(type), out(), in() {}
		
		ConversionNode(const ConversionNode&) = default;
		ConversionNode(ConversionNode&&) = default;
		ConversionNode& operator= (const ConversionNode&) = default;
		ConversionNode& operator= (ConversionNode&&) = default;
	};
	
private:
	/// Storage for conversions
	std::vector<Conversion> edges;
	/// Storage for underlying conversions
	TypeMap<ConversionNode> nodes;
	
	/// Returns the node for ty, creating one if none exists
	ConversionNode& try_insert( const Type* ty ) {
		auto it = nodes.find( ty );
		return ( it == nodes.end()
					? nodes.insert( ty, ConversionNode{ ty } ).first 
					: it )->second;
	}
public:
	/// Iterator for conversion lists
	class const_iterator : public std::iterator<
			std::random_access_iterator_tag, 
			Conversion,
			ConversionList::difference_type,
			const Conversion*,
			const Conversion&> {
	friend class ConversionGraph;

		const ConversionGraph& g;           ///< Graph iterated
		ConversionList::const_iterator it;  ///< Storage iterator

		const_iterator(const ConversionGraph& g) : g(g), it() {}
		const_iterator(const ConversionGraph& g, ConversionList::const_iterator it) : g(g), it(it) {}
	
	public:
		const_iterator() : g(*(const ConversionGraph*)0), it() {}
		const_iterator(const const_iterator&) = default;
		const_iterator& operator= (const const_iterator&) = default;

		const Conversion& operator* () const { return g.edges[*it]; }
		const Conversion* operator-> () const { return &g.edges[*it]; }

		const_iterator& operator++ () { ++it; return *this; }
		const_iterator operator++ (int) { const_iterator tmp = *this; ++(*this); return tmp; }

		const_iterator& operator-- () { --it; return *this; }
		const_iterator operator-- (int) { const_iterator tmp = *this; --(*this); return tmp; }

		const_iterator& operator+= (ConversionList::difference_type i) { it += i; return *this; }
		const_iterator operator+ (ConversionList::difference_type i) const {
			const_iterator tmp = *this; return tmp += i;
		}

		const_iterator& operator-= (ConversionList::difference_type i) { it -= i; return *this; }
		const_iterator operator- (ConversionList::difference_type i) const {
			const_iterator tmp = *this; return tmp -= i;
		}
		ConversionList::difference_type operator- (const const_iterator& o) const {
			return it - o.it;
		}

		const Conversion& operator[] (ConversionList::difference_type i) const {
			return g.edges[*(it + i)];
		}

		bool operator== (const const_iterator& o) const { return it == o.it; }
		bool operator!= (const const_iterator& o) const { return it != o.it; }
		bool operator< (const const_iterator& o) const { return it < o.it; }
		bool operator> (const const_iterator& o) const { return it > o.it; }
		bool operator<= (const const_iterator& o) const { return it <= o.it; }
		bool operator>= (const const_iterator& o) const { return it >= o.it; }
	};

	/// Alias for const_iterator
	using iterator = const_iterator;

	/// Returns a range containing all the conversions from ty
	const std::pair<const_iterator, const_iterator> find_from( const Type* ty ) const {
		auto it = nodes.find( ty );
		if ( it == nodes.end() ) {
			return { const_iterator{*this}, const_iterator{*this} };
		} else {
			return { const_iterator{*this, it->second.out.begin()}, 
			         const_iterator{*this, it->second.out.end()} };
		}
	}

	/// Returns a list of all the conversions to ty
	const std::pair<const_iterator, const_iterator> find_to( const Type* ty ) const {
		auto it = nodes.find( ty );
		if ( it == nodes.end() ) {
			return { const_iterator{*this}, const_iterator{*this} };
		} else {
			return { const_iterator{*this, it->second.in.begin()}, 
			         const_iterator{*this, it->second.in.end()} };
		}
	}
	
	/// Sets up a new conversion 
	void insert( const Type* from, const Type* to, Cost cost ) {
		// Get nodes for these types
		ConversionNode& fromNode = try_insert( from );
		ConversionNode& toNode = try_insert( to );
		// Make conversion edge
		ConversionRef i = edges.size();
		edges.emplace_back( &toNode, &fromNode, cost );
		// Add to both nodes
		fromNode.out.push_back( i );
		toNode.in.push_back( i );
	}
};

using Conversion = ConversionGraph::Conversion;

/// Make a graph of conversions from an existing set of concrete types
ConversionGraph make_conversions( CanonicalTypeMap& types );
