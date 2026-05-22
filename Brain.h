#pragma once
#include <string>
#include <vector>
#include "llama.h"
#include "RAG_Memory.h"
#include "interface.h"

class Brain {
private:
	// Path to the LLM model file
	const std::string modelPath = "models/Hermes-3-Llama-3.1-8B-Q8_0.gguf"; 
	const std::string SYSTEM_PROMPT = "You are an AI Assistant called G.R.A.I.S. (Generig Retrieving Agentic Inference System) informaly written as Grais, that does function calling, the function you can perform are:  " + Interface::getActionSummary() +
		", you MUST ONLY RESPOND IN JSON format with EVERY ONE of these field: {\n"
		"    \"action\": { \"type\": \"integer\", \"minimum\": 0, \"maximum\": 2 },\n"
		"    \"query\": { \"type\": \"string\" },\n"
		"    \"response\": { \"type\": \"string\" },\n"
		"    \"url\": { \"type\": \"string\" }\n"
		"}";
	// GPU layers used 
	const int gpu_layer = -1; 
	// Number of tokens to generate
	const int output_token = 1024; 
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