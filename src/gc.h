#pragma once

#include <algorithm>
#include <vector>

class GC_Traceable;
class GC_Object;

class GC {
public:
	static GC& get();

	const GC& operator<< (const GC_Traceable*) const;

	void register_object(GC_Object*);

	void collect();
private:
	GC();

	bool mark;
	std::vector<class GC_Object*> objects;
//	size_t max_count;
};

inline void traceAll(const GC& gc) {}

template<typename T, typename... Args>
inline void traceAll(const GC& gc, T& x, Args&... xs) {
	gc << x;
	traceAll(gc, xs...);
}

template<typename... Args>
inline void collect(Args&... roots) {
	GC& gc = GC::get();
	traceAll(gc, roots...);
	gc.collect();
}

class GC_Traceable {
	friend class GC;

	mutable bool mark;
protected:
	virtual void trace(const GC& gc) const {}
};

class GC_Object : public GC_Traceable {
	friend class GC;

protected:
	virtual ~GC_Object() {}
public:
	GC_Object();
};
