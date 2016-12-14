#pragma once

#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "ast.h"
#include "binding.h"
#include "conversion.h"
#include "data.h"
#include "decl.h"
#include "type.h"
#include "type_mutator.h"

/// A resolvable expression
class Expr : public ASTNode {
	friend std::ostream& operator<< (std::ostream&, const Expr&);
public:
	virtual Expr* clone() const = 0;
protected:
	virtual void write(std::ostream& out) const = 0;
};

inline std::ostream& operator<< (std::ostream& out, const Expr& e) {
	e.write(out);
	return out;
};

/// An expression that is already typed
class TypedExpr : public Expr {
public:
	/// Get the resolved type for this expression
	virtual const Type* type() const = 0; 
};

/// A variable expression
class VarExpr : public TypedExpr {
	const ConcType* ty_;  ///< Type of variable expression
public:
	typedef Expr Base;
	
	VarExpr(const ConcType* ty_) : ty_( ty_ ) {}
	
	virtual Expr* clone() const { return new VarExpr( ty_ ); }

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	virtual const ConcType* ty() const { return ty_; }
	
	virtual const Type* type() const { return ty_; }

protected:
	virtual void trace(const GC& gc) const { gc << ty_; }

	virtual void write(std::ostream& out) const { out << *ty_; }
};

/// A cast expression
class CastExpr : public TypedExpr {
	const Expr* arg_;         ///< Argument to the cast
	const Conversion* cast_;  ///< Conversion applied
public:
	typedef Expr Base;
	
	CastExpr(const Expr* arg_, const Conversion* cast_)
		: arg_( arg_ ), cast_( cast_ ) {}
	
	virtual Expr* clone() const { return new CastExpr( arg_, cast_ ); }

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	const Expr* arg() const { return arg_; }
	const Conversion* cast() const { return cast_; }
	
	virtual const Type* type() const { return cast_->to->type; }
	
protected:
	virtual void trace(const GC& gc) const { gc << arg_; }

	virtual void write(std::ostream& out) const {
		out << *arg_ << " => " << *type();
	}
};

/// An untyped function call expression
class FuncExpr : public Expr {
	std::string name_;  ///< Name of function called
	List<Expr> args_;   ///< Arguments to call
public:
	typedef Expr Base;
	
	FuncExpr(const std::string& name_, List<Expr>&& args_)
		: name_( name_ ), args_( move(args_) ) {}
	
	virtual Expr* clone() const {
		return new FuncExpr( name_, copy( args_ ) );
	}

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	const std::string& name() const { return name_; }
	const List<Expr>& args() const { return args_; }

protected:
	virtual void trace(const GC& gc) const { gc << args_; }

	virtual void write(std::ostream& out) const {
		out << name_ << "(";
		for (auto& arg : args_) { out << " " << *arg; } 
		out << " )";
	}
};

/// A typed function call expression
class CallExpr : public TypedExpr {
	const FuncDecl* func_;            ///< Function called
	List<TypedExpr> args_;            ///< Function arguments
	unique_ptr<TypeBinding> forall_;  ///< Binding of type variables to concrete types
	mutable const Type* retType_;     ///< Return type of call, after type substitution

	/// Replaces polymorphic type variables in a return type by either their substitution 
	/// or a branded polymorphic type variable.
	class CallRetMutator : public TypeMutator {
		const TypeBinding& binding;
	public:
		CallRetMutator( const TypeBinding& binding ) : binding(binding) {}

		using TypeMutator::mutate;

		const Type* mutate( const PolyType* orig ) override {
			const Type* sub = binding[ orig->name() ];
			if ( sub ) return sub;
			if ( orig->src() == &binding ) return orig;
			else return orig->clone_bound( &binding );
		}
	};

	const Type* retType() const {
		if ( forall_ && forall_->dirty() ) {
			CallRetMutator m( *forall_ );
			retType_ = m.mutate( retType_ );
		}
		return retType_;
	}
public:
	typedef Expr Base;
	
	CallExpr( const FuncDecl* func_, List<TypedExpr>&& args_ = List<TypedExpr>{} )
		: func_( func_ ), args_( move(args_) ), forall_( make_binding( func_ ) ),
		  retType_( func_->returns() ) {}

	CallExpr( const FuncDecl* func_, const List<TypedExpr>& args_, 
	          const unique_ptr<TypeBinding>& forall_ )
		: func_( func_ ), args_( args_ ), 
		  forall_( forall_ ? new TypeBinding( *forall_ ) : nullptr ),
		  retType_( func_->returns() ) {}

	CallExpr( const FuncDecl* func_, List<TypedExpr>&& args_, unique_ptr<TypeBinding>&& forall_ )
		: func_( func_ ), args_( move(args_) ), forall_( move(forall_) ), 
		  retType_( func_->returns() ) {}
	
	virtual Expr* clone() const {
		return new CallExpr( func_, args_, forall_ );
	}

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	const FuncDecl* func() const { return func_; }
	const List<TypedExpr>& args() const { return args_; }
	const TypeBinding* forall() const { return forall_.get(); };
	
	virtual const Type* type() const { return retType(); }
	
protected:
	virtual void trace(const GC& gc) const { gc << func_ << args_ << forall_.get() << retType_; }

	virtual void write(std::ostream& out) const {
		out << func_->name();
		if ( ! func_->tag().empty() ) { out << "-" << func_->tag(); }
		if ( forall_ && ! forall_->empty() ) { out << *forall_; }
		out << "(";
		for (auto& arg : args_) { out << " " << *arg; }
		out << " )"; 
	}
};

/// A single element of a tuple from an underlying expression
class TupleElementExpr : public TypedExpr {
	const TypedExpr* of_;
	unsigned ind_;
public:
	typedef Expr Base;
	
	TupleElementExpr( const TypedExpr* of_, unsigned ind_ )
		: of_( of_ ), ind_( ind_ ) { assert( is<TupleType>( of_->type() ) ); }
	
	virtual Expr* clone() const { return new TupleElementExpr( of_, ind_ ); }

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	const TypedExpr* of() const { return of_; }
	
	unsigned ind() const { return ind_; }
	
	virtual const Type* type() const {
		return as<TupleType>( of_->type() )->types()[ ind_ ];
	}

protected:
	virtual void trace(const GC& gc) const { gc << of_; }

	virtual void write(std::ostream& out) const {
		if ( ind_ == 0 ) {
			out << *of_ << "[0]";
		} else {
			out << "[" << ind_ << "]";
		}
	}
};

/// A tuple constituted of a list of underlying expressions
class TupleExpr : public TypedExpr {
	List<TypedExpr> els_;
	const TupleType* ty_;

	TupleExpr( const List<TypedExpr>& els_, const TupleType* ty_ ) : els_( els_ ), ty_( ty_ ) {}
public:
	typedef Expr Base;

	TupleExpr( List<TypedExpr>&& els_ ) : els_( move(els_) ) {
		List<Type> tys;
		tys.reserve( els_.size() );
		for ( const TypedExpr* el : els_ ) { tys.push_back( el->type() ); }
		ty_ = new TupleType( move(tys) );
	}

	virtual Expr* clone() const { return new TupleExpr( els_, ty_ ); }

	virtual void accept( Visitor& v ) const { v.visit( this ); }

	const List<TypedExpr>& els() const { return els_; }

	const TupleType* ty() const { return ty_; }

	virtual const Type* type() const { return ty_; }

protected:
	virtual void trace(const GC& gc) const { gc << els_ << ty_; }

	virtual void write(std::ostream& out) const {
		out << "[";
		for ( const TypedExpr* el : els_ ) { out << " " << *el; }
		out << " ]";
	}
};

/// An ambiguous interpretation with a given type
class AmbiguousExpr : public TypedExpr {
	const Expr* expr_;      ///< Expression that is ambiguously resolved
	const Type* type_;      ///< Type of ambiguous expression
	List<TypedExpr> alts_;  ///< Equal-cost alternatives
public:
	typedef Expr Base;
	
	AmbiguousExpr( const Expr* expr_, const Type* type_, List<TypedExpr>&& alts_ = {} )
		: expr_(expr_), type_( type_ ), alts_( move(alts_) ) {}
	
	virtual Expr* clone() const { return new AmbiguousExpr( expr_, type_, copy(alts_) ); }

	virtual void accept( Visitor& v ) const { v.visit( this ); }

	const Expr* expr() const { return expr_; }
	
	const List<TypedExpr>& alts() const { return alts_; }

	virtual const Type* type() const { return type_; }

protected:
	virtual void trace(const GC& gc) const { gc << expr_ << type_ << alts_; }

	virtual void write(std::ostream& out) const {
		out << "<ambiguous resolution of type " << *type_ << " for " << *expr_ << ">";
	}
};
