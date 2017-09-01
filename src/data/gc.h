#pragma once

#include <vector>

class GC_Traceable;
class GC_Object;

/// Manually traced and called garbage collector
class GC {
public:
	/// Gets singleton GC instance
	static GC& get();

	/// Traces a traceable object
	const GC& operator<< (const GC_Traceable*) const;

	/// Adds a new object to garbage collection
	void register_object(GC_Object*);

	/// Use young generation for subsequent new objects
	void new_generation();

	/// Collects the young generation, placing survivors in old generation.
	/// Old generation is used for subsequent new objects.
	void collect_young();

	/// Collects all memory; use old generation afterward.
	void collect();

private:
	GC();

	/// The current collection's mark bit
	bool mark;

	typedef std::vector<class GC_Object*> Generation;
	Generation old;
	Generation young;
	bool using_young;
};

/// Use young generation until next collection
inline void new_generation() { GC::get().new_generation(); }

inline void traceAll(const GC& gc) {}

/// Marks all arguments as live in current generation
template<typename T, typename... Args>
inline void traceAll(const GC& gc, T& x, Args&... xs) {
	gc << x;
	traceAll(gc, xs...);
}

/// Traces young-generation roots and does a young collection
template<typename... Args>
inline void collect_young(Args&... roots) {
	GC& gc = GC::get();
	traceAll(gc, roots...);
	gc.collect_young();
}

/// Traces roots and collects other elements
template<typename... Args>
inline void collect(Args&... roots) {
	GC& gc = GC::get();
	traceAll(gc, roots...);
	gc.collect();
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
	virtual ~GC_Object() {}
public:
	GC_Object();
};
