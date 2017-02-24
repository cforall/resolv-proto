#pragma once

#include <iostream>
#include <string>

/// Generates somewhat-pronouncable names for use in random data generator
namespace NameGen {
    static const char consonants[] = { 'b', 'd', 'f', 'g', 'k', 'l', 'm', 'n', 'p', 'r', 't', 'v', 'w', 'x', 'y', 'z' };

    static const char vowels[] = { 'a', 'i', 'o', 'u' };

    static const char caps[] = { 'T', 'R', 'S', 'U', 'V', 'W', 'Q', 'X', 'Y', 'Z' };

    template<typename T, std::size_t N>
    static constexpr std::size_t len(T (&)[N]) { return N; }

    static const unsigned digit_len = len(consonants) * len(vowels);

    /// Gets the i'th name generated
    static std::string get(unsigned i) {
        // special case for single-consonant names
        if ( i < len(consonants) ) return std::string( 1, consonants[i] );

        // calculate length of name
        unsigned iLeft = i - len(consonants);
        unsigned rLen = digit_len * len(consonants);
        unsigned sLen = 3;
        while ( iLeft >= rLen ) {
            iLeft -= rLen;
            rLen *= digit_len;
            sLen += 2;
        }

        // generate name
        std::string name( sLen, '\0' );

        // final consonant
        --sLen;
        name[ sLen ] = consonants[ i % len(consonants) ];
        i = (i - len(consonants)) / len(consonants);
        while ( sLen > 0 ) {
            sLen -= 2;

            // middle consonant-vowel pairs
            unsigned j = i % digit_len;
            i = (i - digit_len) / digit_len;

            name[ sLen + 1 ] = vowels[ j % len(vowels) ];
            name[ sLen ] = consonants[ j / len(vowels) ];

        }
        
        return name;
    }

    /// Gets the i'th name available by repeating a single capital character
    static std::string get_cap(unsigned i) {
        std::string name( i / len(caps) + 1, caps[i % len(caps)] );
        return name;
    }

    /// Gets the i'th name available by repeating a single lowercase consonant
    static std::string get_lower(unsigned i) {
        std::string name( i / len(consonants) + 1, consonants[i % len(consonants)] );
        return name;
    }
};
