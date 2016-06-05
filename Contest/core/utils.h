#ifndef UTILS_123232323233_HPP
#define UTILS_123232323233_HPP

#include <cstring>
#include <cstdio>
#include <vector>
#include <sstream>

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

template <typename T>
void printVector(const std::vector<T>& vector)
{
	std::stringstream result;
	result << "[";

	for (const T c : vector)
		result << (c + 1) << ", ";

	result << "]";

	printf("Payload = %s\n", result.str().c_str());
}

template <typename T>
void printVector(std::string prefix, const T* start, const T* end)
{
	std::stringstream result;
	result << prefix << "[";

	for (; start <= end; ++start)
		result << (*start) << ", ";

	result << "]";

	printf("%s\n", result.str().c_str());
}

#endif