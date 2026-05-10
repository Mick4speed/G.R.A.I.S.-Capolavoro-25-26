#pragma once
#include "llama.h"
#include <sqlite3.h>
#include <hnswlib/hnswlib.h>


//TODO PASS TO MILVUS in the future for better performance and scalability, but for now we will use SQLite and HNSW for simplicity and ease of implementation
class RAG_Memory
{
private:
	const int NODE_NEIGHBORS = 16; //Number of neighbors to connect in HNSW
	const int VECTOR_DIMENSION = 1024; //Dimension of the std::vector embeddings
	const int MAX_ELEMENTS = 1000000; //Maximum number of elements in the HNSW index
	const int EF_CONSTRUCTION = 200; //EF construction parameter for HNSW
	const int MAX_ELEMENT_RETURN = 7; //Max element to return from the index
	const int GPU_LAYER = -1;
	const std::string modelPath = "models/bge-m3-Q8_0.gguf"; // Path to the LLM model file
	
	llama_model* model; // Pointer to the LLaMA model
	llama_context* ctx;
	sqlite3* db; //Database connection
	hnswlib::InnerProductSpace *space;
	hnswlib::HierarchicalNSW<float>* hnswIndex; //HNSW index for std::vector search

	
	bool existsIndex(std::vector<float>* query);
	bool existsSQLite(int id);
	bool insertToSQLite(int id, std::string chunk, std::string source, int importance); //Insert the chunk and source information into the SQLite database
	bool insertToIndex(int id, std::vector<float>* embedding); //Insert the chunk embedding into the HNSW index
	bool removeFromSQLite(int id);
	bool removeFromIndex(int id);
	std::vector<int> searchIndex(std::vector<float>* query); //Get the ID of the chunk embedding from the HNSW database
	std::pair<std::string, std::string> getChunkByID(int id); //Get the source and the chunk text from the SQLite database using the embedding ID

	int getNextID(); //Get the next available ID for a new chunk in the database
	std::vector<float> embedString(std::string chunk); //Embed the chunk into a std::vector 
	
	bool loadEmbedder();
	bool loadSQLite(); //Load the SQLite database and initialize the connection
	bool loadIndex(); //Load the HNSW index from disk and initialize the index
	bool saveIndex(); //Save the HNSW index to disk
	bool closeSQLite(); //Close the SQLite database connection
public:
	int getCount();
	//Embed the chunk and save it to the database with the source information, and importance score from 0 to 100
	bool saveChunk(std::string chunk, std::string source, int importance);
	std::vector<std::string> search(std::string query);
	//Free memory by removing the least important chunks until the target size is reached, and return the number of chunks removed
	int freeMemory(); 
	//Save the HNSW index to disk and commit any changes to the SQLite database
	bool saveMemory();
	RAG_Memory();
	~RAG_Memory();
};