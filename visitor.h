#pragma once

enum class Visit {
    DONE,  ///< End visits
    CONT,  ///< Continue visiting
    SKIP   ///< Continue visiting, skipping this subtree
};

operator bool( const Visit& v ) { return v != Visit::DONE; }
