#pragma once

#include "gc.h"

class Visitor;

/// Base class for all AST objects
class ASTNode : public GC_Object {
public:
    /// Accept a visitor
    virtual void accept( Visitor &v ) const = 0;
};

class FuncDecl;
class VarDecl;
class VarExpr;
class CastExpr;
class FuncExpr;
class CallExpr;
class TupleElementExpr;
class TupleExpr;
class ConcType;
class PolyType;
class VoidType;
class TupleType;

class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit( const FuncDecl* ) {}
    virtual void visit( const VarDecl* ) {}
    virtual void visit( const VarExpr* ) {}
    virtual void visit( const CastExpr* ) {}
    virtual void visit( const FuncExpr* ) {}
    virtual void visit( const CallExpr* ) {}
    virtual void visit( const TupleElementExpr* ) {}
    virtual void visit( const TupleExpr* ) {}
    virtual void visit( const ConcType* ) {}
    virtual void visit( const PolyType* ) {}
    virtual void visit( const VoidType* ) {}
    virtual void visit( const TupleType* ) {}
};

/// Constructs a visitor of type Vis with the given Args, and runs it on each 
/// element of the collection Coll of ASTNode*
template<typename Coll, typename Vis, typename... Args>
Vis visitAll( Coll& c, Args&&... args ) {
    Vis v( std::forward<Args>(args)... );
    
    for ( ASTNode *o : c ) {
        o->accept( v );
    }

    return v;
}
