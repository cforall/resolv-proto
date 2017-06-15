#include "expr.h"

#include "data/gc.h"
#include "resolver/interpretation.h"

void AmbiguousExpr::trace(const GC& gc) const { gc << expr_ << type_ << alts_; }
