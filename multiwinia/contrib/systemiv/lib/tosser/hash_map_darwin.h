#ifndef INCLUDED_HASH_MAP_DARWIN
#define INCLUDED_HASH_MAP_DARWIN

#include <ext/hash_map>
#include <string>

using __gnu_cxx::hash_map;

namespace __gnu_cxx
{
	template <>
	struct hash<std::string>
	{
		size_t operator()(std::string _s) const
		{
			return hash<const char *>()(_s.c_str());
		}
	};
}

#endif