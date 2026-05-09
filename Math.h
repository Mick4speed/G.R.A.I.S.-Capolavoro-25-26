#pragma once
#include <vector>

class MathGPU {
	public:
		static float calculate_norm(std::vector<float>* vec);
		static void normalize_vector(std::vector<float>* vec);
};

class Math {
public:
	static float calculate_norm(std::vector<float>* vec);
	static void normalize_vector(std::vector<float>* vec);
};