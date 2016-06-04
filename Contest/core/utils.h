#ifndef UTILS_123232323233_HPP
#define UTILS_123232323233_HPP

#include <cstring>

template<typename T>
std::vector<char> to_bytes(const T& payload)
{
	std::vector<char> buffer(sizeof(payload));
	memcpy(buffer.data(), (const void*) &payload, sizeof(payload));
	return buffer;
}


template<typename T, typename IT>
int from_bytes(T& t, IT begin)
{
	memcpy(&t, &*begin, sizeof(T));
}

template <typename T>
void joinVectors(std::vector<T>& target, std::vector<T> source)
{
	target.insert(target.end(), source.begin(), source.end());
}


#endif