#pragma once

#include <iostream>
#include <utility>

#ifdef RP_DEBUG

#define DBG(...) std::cerr << "\t" << __FILE__ << ":" << __LINE__ << " " << __VA_ARGS__ << std::endl

#else // ! RP_DEBUG

#define DBG(...)

#endif // RP_DEBUG

namespace {
    template<typename T>
    struct print_id {
        T operator() ( T x ) { return x; }
    };

    template<typename P>
    struct print_deref {
        auto operator() ( P p ) -> decltype(*p) { return *p; }
    };

    template<typename C, typename F>
    struct collection_printer {
        const C& c;
        F f;

        collection_printer(const C& c, F&& f) : c(c), f(f) {}
        collection_printer(const collection_printer&) = default;
        collection_printer(collection_printer&&) = default;
    };
}

template<typename C, typename F>
std::ostream& operator<< ( std::ostream& out, collection_printer<C, F>&& p ) {
    out << '[';
    auto it = p.c.begin();
    if ( it != p.c.end() ) { out << p.f(*it); ++it; }
    while (it != p.c.end()) { out << ", " << p.f(*it); ++it; }
    out << ']';
    return out;
}

template<typename C>
auto print_all(const C& c) -> collection_printer<C, print_id<decltype(*c.begin())>> {
    using T = decltype(*c.begin());
    return collection_printer<C, print_id<T>>{ c, print_id<T>{} };
}

template<typename C>
auto print_all_deref(const C& c) -> collection_printer<C, print_deref<decltype(*c.begin())>> {
    using P = decltype(*c.begin());
    return collection_printer<C, print_deref<P>>{ c, print_deref<P>{} };
}

template<typename C, typename F>
collection_printer<C, F> print_all(const C& c, F&& f) {
    return collection_printer<C, F>{ c, std::move(f) };
}
