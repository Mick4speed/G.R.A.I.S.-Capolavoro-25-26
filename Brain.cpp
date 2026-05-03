#include "llama.h"
#include <string>
#include <locale>
#include <vector>
#include <iostream> //TODO remove this after testing
#include <boost/algorithm/string/join.hpp>
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
	model_params.use_mmap = true;
	model = llama_model_load_from_file(modelPath.c_str(), model_params);
	if (model==NULL) {
		throw runtime_error("Failed to load the model from file: "+modelPath);
	}
}

int Brain::countTokens(llama_context* context) {
	llama_memory_t memory = llama_get_memory(context);
	return llama_memory_seq_pos_max(memory, 0) + 1;
}

void Brain::shiftContext(llama_context* context, int size_prompt) {
	llama_memory_t memory = llama_get_memory(context);
	llama_memory_seq_rm(memory, 0, size_prompt + 1, size_prompt + delete_token);
	llama_memory_seq_add(memory, 0, size_prompt, -1, -delete_token);
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

llama_context* Brain::createContext() {
	llama_context_params context_params = llama_context_default_params();;
	context_params.n_ctx = 4096; // Set the context size to accommodate the prompt and the output tokens
	context_params.n_batch = 4096; // n_batch is the maximum number of tokens that can be processed in a single call to llama_decode
	llama_context* context = llama_init_from_model(model, context_params); // Create a new context with the model and parameters
	if (context == NULL) {
		throw runtime_error("Failed to initialize the context from the model.");
	}
	return context;
}

string Brain::generateResponse(vector<llama_token> prompt_tokens, llama_context* context, llama_sampler* sampler, const llama_vocab* vocabulary) {
	string response = "";
	llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
	int n_past = countTokens(context);
	for (int i = 0; i < batch.n_tokens; ++i) {
		batch.pos[i] = n_past + i;
	}

	if (llama_decode(context, batch) != 0) {
		llama_free(context);
		throw runtime_error("Failed to decode the prompt.");
	}

	n_past += batch.n_tokens;
	for (int i = 0; i < output_token; i++) {
		llama_token token_ID = llama_sampler_sample(sampler, context, -1); // Sample the next token ID from the logits
		if (llama_vocab_is_eog(vocabulary, token_ID) || countTokens(context) >= llama_n_ctx(context)) {
			break;
		}

		char buf[128];
		int n = llama_token_to_piece(vocabulary, token_ID, buf, sizeof(buf), 0, true); // Convert the token ID to its corresponding text piece
		if (n > 0) {
			response += string(buf, n);
		}

		batch = llama_batch_get_one(&token_ID, 1);
		batch.pos[0] = n_past;

		if (llama_decode(context, batch) != 0) {
			break;
		}
		n_past++;
	}
	return response;
}

string Brain::execAction(int action, string JSON) {
	json jsonResponse = json::parse(JSON);
	switch (action) {
		case 0: {	//Return Text
			return jsonResponse.value("response", "");
		} case 1: {	//Search online
			vector<string> results = Interface::searchOnline(jsonResponse.value("query", ""));
			return boost::algorithm::join(results, ",");
		} case 2: {	//Get Web Page
			return Interface::getWebPage(Interface::getUrlFromQuery(jsonResponse.value("URL", "")));
		} case 3: {	//Get Sanitized Page
			return Interface::sanitizePage(Interface::getWebPage(Interface::getUrlFromQuery(jsonResponse.value("URL", ""))));
		} default:
				return "Unknown Action";
	}
}

string Brain::execPrompt(string prompt) {
	//Add prompt instruction and format
	string initial_prompt = "[INST] This is the user prompt: " + prompt + ". You are a friendly and empathic AI, which purpose is to help your master. You can do the sequent action: "+Interface::getActionSummary() +
		"Communicate ONLY in JSON format with an 'action' field, which is the number of the action you want to perform based on the previous action list. And you need to add one or more additional field with name and type equal to the input of the chosen action"
		", if you want to reply be aware that you may have to search on the internet to have the right information before givin an answer"
		"The JSON MUST start with '{' and finish with '}'[/ INST]";
	

	const llama_vocab* vocabulary = llama_model_get_vocab(model); // Get the model's vocabulary
	llama_sampler* sampler = getSampler(); // Initialize the sampler with default parameters

	const int size_initial_prompt = -llama_tokenize(vocabulary, prompt.c_str(), prompt.size(), NULL, 0, true, true); //Calculate the number of tokens in the prompt
	string actual_prompt = initial_prompt;
	int current_action = -1;
	llama_context* context = nullptr;
	string response = "";

	do {
		bool isFirst = context == nullptr;
		const int size_prompt = -llama_tokenize(vocabulary, actual_prompt.c_str(), actual_prompt.size(), NULL, 0, isFirst, isFirst); //Calculate the number of tokens in the prompt
	
		vector<llama_token> prompt_tokens(size_prompt); // Create a vector to hold the tokenized prompt
		if (llama_tokenize(vocabulary, actual_prompt.c_str(), actual_prompt.size(), prompt_tokens.data(), prompt_tokens.size(), isFirst, isFirst) < 0) {
			throw runtime_error("Failed to tokenize the prompt: " + actual_prompt);
		}
		if (context==nullptr) context = createContext();

		do {
			response = generateResponse(prompt_tokens, context, sampler, vocabulary);
		} while (cleanResponse(response) == "");
		cout << response << endl; //TODO remove this after testing

		json jsonResponse = json::parse(response);
		current_action = jsonResponse["action"];
		string result = execAction(current_action, response);
		
		if (current_action == 0) response = result; //If the action is 0, we want to return the response to the user
		
		actual_prompt = "[RESULT]" + result + "[/RESULT]\n[INST]Continue based on the result above. [/INST]\n";
		if (countTokens(context) > llama_n_ctx(context) - 500) {
			shiftContext(context, size_initial_prompt);
		}
	} while (current_action > 0);


	llama_free(context);
	llama_sampler_free(sampler);
	return response;
}

Brain::~Brain() {
	if (model != nullptr) {
		llama_model_free(model);
	}
	llama_backend_free();
}