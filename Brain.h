#pragma once
#include <string>
#include <vector>
#include "llama.h"

class Brain {
private:
	std::string modelPath = "models/Mistral-Nemo-12B-ArliAI-RPMax-v1.1-Q5_K_M.gguf"; // Path to the LLM model file
	int gpu_layer = -1; // Use all available GPU layers
	int output_token = 1024; // Number of tokens to generate
	int n_past = 0;
	llama_model* model; // Pointer to the LLaMA model
	llama_sampler* getSampler();
	std::string cleanResponse(std::string);
	int getMaxContextSize(int);
	llama_context* createContext(int);
	std::string generateResponse(std::vector<llama_token>, llama_context*, llama_sampler*, const llama_vocab*, int);
public:
	Brain();
	~Brain();
	std::string execPrompt(std::string);
};