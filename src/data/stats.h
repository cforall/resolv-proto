#pragma once

#ifdef RP_STATS

/// Enumeration of passes
enum pass { Parse, Resolve, N_PASSES };

extern pass crnt_pass;  ///< Current pass

extern unsigned allocs_in[N_PASSES];  ///< Number of AST allocations per pass

#endif // RP_STATS
