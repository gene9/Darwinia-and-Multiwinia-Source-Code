#ifndef __MAP_UTILS_H
#define __MAP_UTILS_H

#include <vector>
#include <map>

template <class K, class V>
inline
K fst( const std::pair< K, V > &_pair )
{
	return _pair.first;
}

template <class K, class V>
inline
K snd( const std::pair< K, V > &_pair )
{
	return _pair.second;
}

template <class K, class V>
inline
std::vector< K > Keys( const std::map< K, V > _map )
{
	std::vector< K > result;
	std::transform( _map.begin(), _map.end(), std::back_inserter( result ), fst< K, V > );
	return result;
}

template <class K, class V>
inline
std::vector< K > Values( const std::map< K, V > _map )
{
	std::vector< K > result;
	std::transform( _map.begin(), _map.end(), std::back_inserter( result ), snd< K, V > );
	return result;
}

#endif // MAP_UTILS_H
