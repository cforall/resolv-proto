#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <cassert>
#include <vector>

class GC_Traceable;
class GC_Object;
class GC_Guard;

/// Manually traced and called garbage collector
class GC {
	friend class GC_Guard;

	/// Collects the youngest generation, placing survivors in previous generation.
	/// Young generation objects cannot be kept alive by pointers from older generation.
	/// Older generation is used for subsequent new objects.
	void collect_young();
public:
	/// Gets singleton GC instance
	static GC& get();

	/// Traces a traceable object
	const GC& operator<< (const GC_Traceable*) const;

	/// Adds a new object to garbage collection
	void register_object(GC_Object*);

	/// Adds an object to the set of static roots
	void register_static_root(GC_Traceable*);

	/// Start new generation for subsequent new objects
	GC_Guard new_generation();

	/// Traces all static roots
	void trace_static_roots();

	/// Collects oldest generation; use oldest generation afterward.
	/// Error if currently using younger generation
	void collect();

	/// Collects all contained objects
	~GC();

private:
	GC();

	using Generation = std::vector<GC_Object*>;
	std::vector<Generation> gens;  ///< Set of generations; always at least one

	using StaticRoots = std::vector<GC_Traceable*>;
	StaticRoots static_roots;      ///< Set of static-lifetime roots

	bool mark;                     ///< The current collection's mark bit
	unsigned g;                    ///< The current number generation in use

	/// Trace a traceable object (enabled by SFINAE)
	template<typename T>
	static inline auto do_trace(const GC& gc, T& obj, int) -> decltype(gc << obj, void()) {
		gc << obj;
	}

	/// Do not trace an untraceable object
	template<typename T>
	static inline auto do_trace(const GC&, T&, long) -> void {}

	/// Base case for maybe_trace
	void maybe_trace() const {}

public:
	/// Trace any objects that are traceable
	template<typename T, typename... Args>
	void maybe_trace(T& obj, Args&... args) const {
		// uses SFINAE trick to select proper overload; prefers actually tracing version 
		// (because of int->long conversion), but will fall back to non-tracing
		do_trace(*this, obj, 0);
		maybe_trace(args...);
	}
};

/// Cleanup object for young generation
class GC_Guard {
	friend class GC;

	GC& gc;      ///< GC associated with
	unsigned g;  ///< Generation constructed for

	GC_Guard( GC& gc, unsigned g ) : gc(gc), g(g) {}

public:
	~GC_Guard() {
		assert( gc.g == g && "collecting current generation" );
		gc.collect_young();
	}
};

/// Use young generation until next collection
inline GC_Guard new_generation() { return GC::get().new_generation(); }

inline void traceAll(const GC&) {}

/// Marks all arguments as live in current generation
template<typename T, typename... Args>
inline void traceAll(const GC& gc, T& x, Args&... xs) {
	gc << x;
	traceAll(gc, xs...);
}

/// Traces roots without collecting
template<typename... Args>
inline void trace(Args&... roots) {
	GC& gc = GC::get();
	traceAll(gc, roots...);
}

/// Traces roots and collects other elements; should not be any young generations live
template<typename... Args>
inline void collect(Args&... roots) {
	GC& gc = GC::get();
	traceAll(gc, roots...);
	gc.collect();
}

/// Makes a new expression as a static root
template<typename T, typename... Args>
inline T* new_static_root( Args&&... args ) {
	T* root = new T( std::forward<Args>(args)... );
	GC::get().register_static_root( root );
	return root;
}

/// Class that is traced by the GC, but not managed by it
class GC_Traceable {
	friend class GC;

	mutable bool mark;
protected:
	/// override to trace any child objects
	virtual void trace(const GC& gc) const {}
};

/// Class that is managed by the GC
class GC_Object : public GC_Traceable {
	friend class GC;
protected:
	// Override default constructors to ensure clones are registered and properly marked
	GC_Object();

	GC_Object(const GC_Object&);

	GC_Object(GC_Object&&);

	GC_Object& operator= (const GC_Object&) { /* do not assign mark */ return *this; }

	GC_Object& operator= (GC_Object&&) { /* do not assign mark */ return *this; }

	// Ensure subclasses can be deleted by garbage collector
	virtual ~GC_Object() {}
};
