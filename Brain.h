#pragma once
#include <string>
#include <vector>
#include "llama.h"
#include "RAG_Memory.h"
#include "interface.h"

class Brain {
private:
	const std::string modelPath = "models/Hermes-3-Llama-3.1-8B-Q8_0.gguf"; // Path to the LLM model file
	const std::string SYSTEM_PROMPT = "You are an AI Assistant that does function calling, the function you can perform are:  " + Interface::getActionSummary() +
		", you MUST ONLY RESPOND IN JSON format with EVERY ONE of these field: {\n"
		"    \"action\": { \"type\": \"integer\", \"minimum\": 0, \"maximum\": 2 },\n"
		"    \"query\": { \"type\": \"string\" },\n"
		"    \"response\": { \"type\": \"string\" },\n"
		"    \"url\": { \"type\": \"string\" }\n"
		"}";
	const int gpu_layer = -1; // Use all available GPU layers
	const int output_token = 1024; // Number of tokens to generate
	const int delete_token = 1024; // Number of tokens to delete when the context is full
	llama_model* model; // Pointer to the LLaMA model
	RAG_Memory rag;
	llama_sampler* getSampler();
	std::vector<llama_token> tokenizeString(std::string STRING, bool is_first_turn);
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