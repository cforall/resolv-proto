// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include "expr.h"

#include "data/gc.h"
#include "resolver/interpretation.h"

void AmbiguousExpr::trace(const GC& gc) const { gc << expr_ << type_ << alts_; }
