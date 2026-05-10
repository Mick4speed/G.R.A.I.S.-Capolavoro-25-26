#include "Math.h"
#include <vector>
using namespace std;

float Math::calculate_norm(vector<float>* vec) {
	float res=0;
	for (float i : *vec) {
		res += i * i;
	}
	return sqrt(res);
}

void Math::normalize_vector(vector<float> * vec) {
	if (vec->empty()) return;
	float norm = calculate_norm(vec);
	if (norm == 0) return;
	for (int i = 0;i < vec->size(); i++) {
		vec->at(i) = vec->at(i) / norm;
	}
}