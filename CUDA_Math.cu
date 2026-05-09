#include "Math.h"
#include <vector>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
using namespace std;

__global__ void square(float* input, int n) {
	unsigned int ID = threadIdx.x;
	if (ID<n) {
		//y=x^2
		input[ID] = (input[ID] * input[ID]);
	}
}

__global__ void sum(float* input, float* output, int n) {
	unsigned int ID = threadIdx.x;
	if (ID < n && ID % 2 == 0) {
		if (ID + 1 < n)
			output[ID / 2] = input[ID] + input[ID + 1];
		else
			output[ID / 2] = input[ID]; // Gestione n dispari
	}
}

__global__ void divideByANumber(float* arr, float div, int n) {
	unsigned int ID = threadIdx.x;
	if (ID < n) {
		//y=x^2
		arr[ID] = arr[ID]/div;
	}
}

float MathGPU::calculate_norm(vector<float>* vec) {
	float *input, *output;
	cudaMalloc(&input, vec->size() * sizeof(float));
	cudaMalloc(&output, vec->size() * sizeof(float));
	cudaMemcpy(input, vec->data(), vec->size()*sizeof(float), cudaMemcpyHostToDevice);
	int n = vec->size();
	square << <1, n >> > (input, n);
	while (n > 1) {
		sum<<<1, n>>>(input, output, n);
		cudaDeviceSynchronize();
		//change vector
		float* buf = input;
		input = output;
		output = buf;
		n = (n + 1) / 2;
	}
	float result;
	cudaMemcpy(&result, input, sizeof(float), cudaMemcpyDeviceToHost);
	cudaFree(input);
	cudaFree(output);
	return (float)sqrt(result);
}

void MathGPU::normalize_vector(vector<float>* vec) {
	if (vec->empty()) return;
	float norm = calculate_norm(vec);
	if (norm == 0) return;
	float* data;
	cudaMalloc(&data, vec->size() * sizeof(float));
	cudaMemcpy(data, vec->data(), vec->size() * sizeof(float), cudaMemcpyHostToDevice);
	divideByANumber << <1, vec->size() >> > (data, norm, vec->size());
	cudaDeviceSynchronize();
	cudaMemcpy(vec->data(), data, vec->size() * sizeof(float), cudaMemcpyDeviceToHost);
	cudaFree(data);
}