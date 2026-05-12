#pragma once
#include <string>
#include <vector>
#include "llama.h"
#include "RAG_Memory.h"

class Brain {
private:
	std::string modelPath = "models/Mistral-Nemo-12B-ArliAI-RPMax-v1.1-Q5_K_M.gguf"; // Path to the LLM model file
	int gpu_layer = -1; // Use all available GPU layers
	const int output_token = 1024; // Number of tokens to generate
	const int delete_token = 1024; // Number of tokens to delete when the context is full
	llama_model* model; // Pointer to the LLaMA model
	llama_sampler* getSampler();
	RAG_Memory rag;
	std::string cleanResponse(std::string);
	int getMaxContextSize(int);
	llama_context* createContext();
	std::string generateResponse(std::vector<llama_token>, llama_context*, llama_sampler*, const llama_vocab*);
	std::string execAction(int, std::string);
	void shiftContext(llama_context*, int);
	int countTokens(llama_context*);
public:
	Brain();
	~Brain();
	std::string execPrompt(std::string);
};