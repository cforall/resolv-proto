#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>

int main() {
	using namespace std;

	regex type_ids("<([0-9]+)>");  ///< match the ID tags on polymorphic type variables
	sregex_token_iterator eos{};   ///< end-of-string iterator

	// loop over lines
	string line;
	unordered_map<string, unsigned> subs;
	while ( getline( cin, line ) ) {
		// loop over matches of the inner tag
		sregex_token_iterator it{ line.begin(), line.end(), type_ids, 1 };
		string::const_iterator last = line.cbegin();  ///< last non-matching character
		for ( ; it != eos; ++it ) {
			/// string up to start of match
			string pretoken( last, it->first );
			// replace tag by sequentially-assigned tag
			string token = *it;
			auto sub = subs.find( token );
			if ( sub == subs.end() ) {
				sub = subs.emplace_hint( sub, token, subs.size() );
			}
			// print up to end of replaced tag, and iterate
			cout << pretoken << sub->second;
			last = it->second;
		}
		// print end of string and reset substitutions
		string posttoken( last, line.cend() );
		cout << posttoken << endl;
		subs.clear();
	}
}