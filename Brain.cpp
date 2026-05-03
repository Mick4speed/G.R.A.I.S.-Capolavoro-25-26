#include "llama.h"
#include <string>
#include <locale>
#include <vector>
#include <iostream> //TODO remove this after testing
#include "Brain.h"
#include "interface.h"
#include "json.hpp"
using namespace std;
using json = nlohmann::json;
	

Brain::Brain(){
	//TODO remove this when we find a better way to make the user interact with the AI
	auto silent_log = [](ggml_log_level level, const char* text, void* user_data) {
		(void)level; (void)user_data;
		// Non facciamo nulla, quindi il testo non viene stampato
		};

	// 2. Diciamo a llama.cpp di usare questa funzione
	llama_log_set(silent_log, nullptr);

	llama_backend_init();
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = gpu_layer;
	model = llama_model_load_from_file(modelPath.c_str(), model_params);
	if (model==NULL) {
		throw runtime_error("Failed to load the model from file: "+modelPath);
	}
}

llama_sampler* Brain::getSampler() {
	llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params()); // Initialize the sampler with default parameters
	llama_sampler_chain_add(sampler, llama_sampler_init_penalties(64, 1.1f, 0.0f, 0.0f));	
	llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.7f)); 
	llama_sampler_chain_add(sampler, llama_sampler_init_dist(time(NULL)));
	return sampler;
}

string Brain::cleanResponse(string response) {
    size_t l = response.find_first_of("{");
    size_t r = response.find_last_of("}");
    if (l == string::npos || r == string::npos || r < l) {
		return "";
    }
    return response.substr(l, r - l + 1);
}

int Brain::getMaxContextSize(int size_prompt) {
	return 2*(size_prompt + output_token);
}

llama_context* Brain::createContext(int size_prompt) {
	llama_context_params context_params = llama_context_default_params();;
	context_params.n_ctx = getMaxContextSize(size_prompt); // Set the context size to accommodate the prompt and the output tokens
	context_params.n_batch = size_prompt; // n_batch is the maximum number of tokens that can be processed in a single call to llama_decode
	llama_context* context = llama_init_from_model(model, context_params); // Create a new context with the model and parameters
	if (context == NULL) {
		throw runtime_error("Failed to initialize the context from the model.");
	}
	return context;
}

string Brain::generateResponse(vector<llama_token> prompt_tokens, llama_context* context, llama_sampler* sampler, const llama_vocab* vocabulary, int size_prompt) {
	string response = "";
	llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
	if (llama_decode(context, batch) != 0) {
		llama_free(context);
		throw runtime_error("Failed to decode the prompt.");
	}

	uint32_t n_cur = batch.n_tokens;
	cout << n_cur << endl;
	for (int i = 0; i < output_token; i++) {
		float* logits = llama_get_logits_ith(context, batch.n_tokens - 1);
		llama_token token_ID = llama_sampler_sample(sampler, context, -1); // Sample the next token ID from the logits
		if (llama_vocab_is_eog(vocabulary, token_ID) || n_cur >= getMaxContextSize(size_prompt)) {
			break;
		}

		char buf[128];
		int n = llama_token_to_piece(vocabulary, token_ID, buf, sizeof(buf), 0, true); // Convert the token ID to its corresponding text piece
		if (n > 0) {
			response += string(buf, n);
		}

		batch = llama_batch_get_one(&token_ID, 1);

		if (llama_decode(context, batch) != 0) {
			break;
		}
		n_cur++;
	}
	return response;
}

string Brain::execPrompt(string prompt) {
	//Add prompt instruction and format
	prompt = "[INST] This is the user prompt: " + prompt + ". You are a friendly and empathic AI, which purpose is to help your master. You can do the sequent action: "+Interface::getActionSummary() +
		"Communicate ONLY in JSON format with an 'action' field, which is the number of the action you want to perform based on the previous action list. And you need to add one or more additional field with name and type equal to the input of the chosen action"
		", if you want to reply be aware that you may should search on the internet to have the right information before givin an answer"
		"The JSON MUST start with '{' and finish with '}'[/ INST]";
	
	const llama_vocab* vocabulary = llama_model_get_vocab(model); // Get the model's vocabulary
	const int size_prompt = -llama_tokenize(vocabulary, prompt.c_str(), prompt.size(), NULL, 0, true, true); //Calculate the number of tokens in the prompt
	llama_sampler* sampler = getSampler(); // Initialize the sampler with default parameters
	
	vector<llama_token> prompt_tokens(size_prompt); // Create a vector to hold the tokenized prompt
	if (llama_tokenize(vocabulary, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true) < 0) {
		throw runtime_error("Failed to tokenize the prompt: " + prompt);
	}
	
	llama_context* context = createContext(size_prompt);

	string response = "";
	do {
		response = generateResponse(prompt_tokens, context, sampler, vocabulary, size_prompt);
	} while (cleanResponse(response)=="");
	llama_free(context);
	llama_sampler_free(sampler);
	return cleanResponse(response);
}

Brain::~Brain() {
	if (model != nullptr) {
		llama_model_free(model);
	}
	llama_backend_free();
}