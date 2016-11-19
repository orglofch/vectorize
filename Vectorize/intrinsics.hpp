#ifndef _INTRINSICS_HPP_
#define _INTRINSICS_HPP_

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#ifndef _DEBUG
#define _DEBUG 0
#endif

// TODO(orglofch): Possible remove preprocessor error suppression

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define LOG(fmt, ...) do { if (_DEBUG) { fprintf(stderr, "%s:%d: " fmt, __FILENAME__, __LINE__, __VA_ARGS__); } } while(0)

inline
std::string ReadFile(const char *filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		LOG("Unable to open %s\n", filename);
		return "";
	}

	std::stringstream file_data;
	file_data << file.rdbuf();
	file.close();

	return file_data.str();
}

inline
void clamp(float *val, float min, float max) {
	*val = std::min(max, std::max(min, *val));
}

inline
void clamp(int *val, int min, int max) {
	*val = std::min(max, std::max(min, *val));
}

inline
float Randf(float min, float max) {
	return (max - min) * rand() / RAND_MAX + min;
}

#endif