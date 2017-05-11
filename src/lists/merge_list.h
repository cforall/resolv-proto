#pragma once

#include <utility>
#include <vector>

#include "nway_merge.h"

/// Wraps an NWayMerge with a cached output queue with a std::vector-compatible 
/// interface.
///
/// Template parameters have same semantics and defaults as NWayMerge.
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 typename Q = std::vector<T>,
		 typename Valid = always_valid< Q >,
		 template<typename> class OutQ = std_priority_queue>
class MergeList {
public:
	typedef std::pair< K, std::vector<T> > value_type;
	typedef std::vector< value_type > queue_type;
	typedef typename queue_type::size_type size_type;
	typedef typename queue_type::difference_type difference_type;
	typedef typename queue_type::reference reference;
	typedef typename queue_type::const_reference const_reference;
	typedef typename queue_type::pointer pointer;
	typedef typename queue_type::const_pointer const_pointer;
private:
	typedef NWayMerge<T, K, Extract, Q, Valid, OutQ> Merge;
	 
};
