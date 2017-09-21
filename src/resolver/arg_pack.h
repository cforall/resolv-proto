#pragma once

#include "cost.h"
#include "env.h"
#include "interpretation.h"

#include "ast/expr.h"
#include "data/list.h"

/// State to iteratively build a match of expressions to arguments
struct ArgPack {
	const Env* env;                   ///< Current environment
	Cost cost;                        ///< Current cost
	Cost argCost;                     ///< Current argument-only cost
	List<TypedExpr> args;             ///< List of current arguments
	const TypedExpr* crnt;            ///< Current expression being built (nullptr for on_last == 0)
	unsigned on_last;                 ///< Number of unpaired type atoms on final arg
#if defined RP_MODE_TD
	List<Expr>::const_iterator next;  ///< Next argument expression
#elif defined RP_MODE_BU
	unsigned next;                    ///< Next argument index
#endif

	ArgPack() = default;
	
	#if defined RP_MODE_TD
	/// Initialize ArgPack with first argument iterator and initial environment
	ArgPack(const List<Expr>::const_iterator& it, const Env* e) 
		: env(e), cost(), argCost(), args(), crnt(nullptr), on_last(0), next(it) {}
	#elif defined RP_MODE_BU
	ArgPack(const Env* e)
		:env(e), cost(), argCost(), args(), crnt(nullptr), on_last(0), next(0) {}
	#endif
	
	/// Update ArgPack with new interpretation for next arg
	ArgPack(const ArgPack& old, const Interpretation* i, unsigned leftover = 0)
		: env(i->env), cost(old.cost + i->cost), argCost(old.argCost + i->argCost), 
		  args(old.args), crnt(leftover ? i->expr : nullptr), on_last(leftover), next(old.next) {
		if ( old.crnt ) args.push_back( old.crnt );
		if ( on_last == 0 ) args.push_back( i->expr );
		++next;
	}

	/// Update ArgPack with new interpretation for next arg, specifying cost and environment
	ArgPack(const ArgPack& old, const Interpretation* i, Cost&& newCost, const Env* newEnv, 
			unsigned leftover = 0 )
		: env(newEnv), cost(move(newCost)), argCost(old.argCost + i->argCost),
		  args(old.args), crnt(leftover ? i->expr : nullptr), on_last(leftover), next(old.next) {
		if ( old.crnt ) args.push_back( old.crnt );
		if ( on_last == 0 ) args.push_back( i->expr );
		++next;
	}

	/// Update ArgPack with new binding for leftover arg portion
	ArgPack(const ArgPack& old, Cost&& newCost, const Env* newEnv, unsigned leftover)
		: env(newEnv), cost(move(newCost)), argCost(old.argCost), args(old.args), 
		  crnt(leftover ? old.crnt : nullptr), on_last(leftover), next(old.next) {
		if ( on_last == 0 ) args.push_back( old.crnt );
	}

	/// Truncate crnt if applicable to exclude on_last, add to arguments
	void truncate() {
		if ( on_last > 0 ) {
			unsigned trunc = crnt->type()->size() - on_last;
			args.push_back( new TruncateExpr{ crnt, trunc } );
			cost.safe += trunc;
			crnt = nullptr;
		}
	}
};
