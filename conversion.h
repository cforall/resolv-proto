#pragma once

#ifdef RP_SORTED
#include <map>
#else
#include <unordered_map>
#endif

#include <ostream>
#include <vector>

#include "cost.h"
#include "type.h"
#include "utility.h"

/// Graph of conversions
class ConversionGraph {
	friend std::ostream& operator<< (std::ostream&, const ConversionGraph&);
public:
	struct ConversionNode;
	
	/// Edge in the conversion graph representing a direct conversion
	struct Conversion {
		Ref<ConversionNode> to;  ///< Type converted to
		Cost cost;               ///< Cost of this conversion
		
		Conversion() = default;
		Conversion(Ref<ConversionNode> to, Cost cost) : to(to), cost(cost) {}
		
		Conversion(const Conversion&) = default;
		Conversion(Conversion&&) = default;
		Conversion& operator= (const Conversion&) = default;
		Conversion& operator= (Conversion&&) = default;
	};
	
	typedef std::vector<Conversion> ConversionList;
	
	/// Node in the conversion graph representing one type
	struct ConversionNode {
		Ref<Type> type;              ///< Type to convert from
		ConversionList conversions;  ///< Types directly convertable from this one
		
		ConversionNode() = default;
		ConversionNode(Ref<Type> type) : type(type), conversions() {}
		
		ConversionNode(const ConversionNode&) = default;
		ConversionNode(ConversionNode&&) = default;
		ConversionNode& operator= (const ConversionNode&) = default;
		ConversionNode& operator= (ConversionNode&&) = default;
	};
	
private:
#ifdef RP_SORTED
	template<typename K, typename V>
	using NodeMap = std::map<K, V>;
#else
	template<typename K, typename V>
	using NodeMap = std::unordered_map<K, V>;
#endif
	/// Dummy empty conversion list
	static const ConversionList no_conversions;
	/// Storage for underlying conversions
	NodeMap< Ref<Type>, ConversionNode > nodes;
	
	/// Returns the node for ty, creating one if none exists
	ConversionNode& try_insert( Ref<Type> ty ) {
		auto it = nodes.find( ty );
		return ( it == nodes.end() ?
			nodes.emplace_hint( it, ty, ConversionNode{ ty } ) :
			it )->second;
	}
public:
	/// Returns a list of all the conversions for ty
	const ConversionList& find( Ref<Type> ty ) const {
		auto it = nodes.find( ty );
		return it == nodes.end() ? no_conversions : it->second.conversions;
	}
	
	/// Sets up a new conversion 
	void insert( Ref<Type> from, Ref<Type> to, Cost cost ) {
		ConversionNode& fromNode = try_insert( from );
		ConversionNode& toNode = try_insert( to );
		
		fromNode.conversions.emplace_back( ref(toNode), cost );
	}
};

/// Make a graph of conversions from an existing set of concrete types
ConversionGraph make_conversions( const SortedSet<ConcType>& types );
