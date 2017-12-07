#pragma once

#include "cost.h"
#include "env.h"

#include "ast/type.h"
#include "ast/mutator.h"
#include "ast/type_mutator.h"
#include "data/cast.h"
#include "data/guard.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"

/// Unifies two types, returning their most specific common type
class TypeUnifier : public TypeMutator<TypeUnifier> {
public:
	using Parent = TypeMutator<TypeUnifier>;

	Env* env;             ///< Environment to unify under
private:
	using TyIt = option<List<Type>::const_iterator>;
	
	Cost::Element& cost;  ///< Number of bindings performed
	const Type* to;       ///< Type to unify to
	TyIt toIt;            ///< Iterator generating list of unification targets
	bool changedTo;       ///< Has the target type been changed?

	const Type* unifyTo() {
		if ( toIt ) {
			List<Type>::const_iterator& it = *toIt;
			const Type* t = *it;
			++it;
			return t;
		} else {
			return to;
		}
	}

	/// Unifies a concrete/named parameter with a polymorphic target, 
	/// returns success and sets r to common type or nullptr if none such
	bool unifyConcPoly( const Type* t, const PolyType* u, const Type*& r ) {
		ClassRef uc = getClass( env, u );

		if ( uc->bound ) {
			// recursive call on bound type
			changedTo = false;
			auto guard = swap_in_scope( to, copy(uc->bound) );
			auto guard2 = swap_in_scope( toIt, TyIt{} );
			r = Parent::operator()( t );

			// fail early
			if ( r == nullptr ) return false;
		} else {
			// unbound class can just bind to concrete parameter
			r = t;
		}
		// mark binding, changed target type
		changedTo = true;
		++cost;
		uc = getClass( env, u );  // reacquire in case of iterator invalidation
		return bindType( env, uc, r );
	}

public:
	TypeUnifier( Env* env, Cost::Element& cost )
		: env(env), cost(cost), to(nullptr), toIt(), changedTo(false) {}

	using Parent::visit;

	bool visit( const ConcType* t, const Type*& r ) {
		const Type* ut = unifyTo();
		auto uid = typeof(ut);
		if ( uid == typeof<ConcType>() ) {
			if ( *t == *as<ConcType>(ut) ) return true;
		} else if ( uid == typeof<PolyType>() ) {
			return unifyConcPoly( t, as<PolyType>(ut), r );
		}

		// no match
		r = nullptr;
		return false;
	}

	bool visit( const NamedType* t, const Type*& r ) {
		const Type* ut = unifyTo();
		auto uid = typeof(ut);
		// handle binding to polymorphic target
		if ( uid == typeof<PolyType>() ) {
			return unifyConcPoly( t, as<PolyType>(ut), r );
		}

		// check compatibly-named NamedType
		if ( uid != typeof<NamedType>() ) {
			r = nullptr;
			return false;
		}

		const NamedType* u = as<NamedType>( ut );
		if ( t->name() != u->name() ) {
			r = nullptr;
			return false;
		}

		// check equal parameter list sizes
		const List<Type>& tps = t->params();
		const List<Type>& ups = u->params();
		if ( tps.size() != ups.size() ) {
			r = nullptr;
			return false;
		}

		// recursively unify 
		bool oldChangedTo = changedTo;
		changedTo = false;
		auto guard = swap_in_scope( toIt, TyIt{ ups.begin() } );

		option<List<Type>> newTypes;
		if ( ! mutateAll( this, tps, newTypes ) ) {
			r = nullptr;
			return false;
		}
		if ( newTypes ) {
			r = changedTo ? new NamedType{ t->name(), *move(newTypes) } : u;
		}

		changedTo |= oldChangedTo;
		return true;
	}

	bool visit( const TupleType* t, const Type*& r ) {
		// check tuple type
		const TupleType* u = as_safe<TupleType>( unifyTo() );
		if ( u == nullptr ) {
			r = nullptr;
			return false;
		}

		// check equal element list sizes
		const List<Type>& tes = t->types();
		const List<Type>& ues = u->types();
		if ( tes.size() != ues.size() ) {
			r = nullptr;
			return false;
		}

		// recursively unify
		bool oldChangedTo = changedTo;
		changedTo = false;
		auto guard = swap_in_scope( toIt, TyIt{ ues.begin() } );

		option<List<Type>> newTypes;
		if ( ! mutateAll( this, tes, newTypes ) ) {
			r = nullptr;
			return false;
		}
		if ( newTypes ) {
			r = changedTo ? new TupleType{ *move(newTypes) } : u;
		}

		changedTo |= oldChangedTo;
		return true;
	}

	bool visit( const PolyType* t, const Type*& r ) {
		ClassRef pc = getClass( env, t );

		const Type* ut = unifyTo();
		auto uid = typeof(ut);
		if ( uid == typeof<ConcType>() || uid == typeof<NamedType>() ) {
			if ( pc->bound ) {
				// recursive call on bound type
				bool oldChangedTo = changedTo;
				changedTo = false;
				r = Parent::operator()( pc->bound );
				changedTo |= oldChangedTo;

				// fail early
				if ( r == nullptr ) return false;
			} else {
				// unbound class can just be bound to concrete target
				r = ut;
			}
			// mark binding
			++cost;
			pc = getClass( env, t );  // reaquire class in case of iterator invalidation
			return bindType( env, pc, r );
		} else if ( uid == typeof<PolyType>() ) {
			// attempt to bind variable; this is the point that may invalidate iterators
			if ( ! bindVar( env, pc, as<PolyType>(ut) ) ) {
				r = nullptr;
				return false;
			}

			// count binding, update representative
			cost += 2;  // to argument + from paramter
			if ( pc->bound ) {
				r = pc->bound;
				changedTo = true;
			}
			return true;
		}

		// fail on unexpected argument (shouldn't reach)
		r = nullptr;
		return false;
	}

	const Type* operator() ( const Type* a, const Type* b ) {
		to = b;
		changedTo = false;
		return Parent::operator()( a );
	}
};
