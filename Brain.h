#pragma once
#include <string>
#include <vector>
#include "llama.h"
#include "PythonRuntime.h"
#include "RAG_Memory.h"
#include "interface.h"

class Brain {
private:
	// Path to the LLM model file
	const std::string modelPath = "models/LLM.gguf"; 
	//Model System prompt //TODO add OS name and user folder path
	std::string SYSTEM_PROMPT;
	// GPU layers used 
	const int gpu_layer = -1; 
	// Number of tokens to generate
	const int output_token = 1024; 
	PythonRuntime python;
	// Inizializzata nel costruttore e immutabile dopo
	int PROMPT_CONTEXT_SIZE; 
	// Pointer to the LLaMA model
	llama_model* model; 
	llama_context* context;
	llama_sampler* sampler;
	RAG_Memory rag;
	llama_sampler* createSampler();
	std::vector<llama_token> tokenizeString(std::string STRING, bool is_first_turn);
	std::string cleanResponse(std::string);
	int getMaxContextSize(int);
	llama_context* createContext();
	std::string generateResponse(std::vector<llama_token>, llama_sampler*, const llama_vocab*);
	std::string execAction(int, std::string);
	void shiftContext(int);
	int countTokens();
public:
	Brain();
	~Brain();
	std::string execPrompt(std::string);
};