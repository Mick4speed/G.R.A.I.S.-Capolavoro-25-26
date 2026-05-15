#pragma once
#include <string>
#include "RAG_Memory.h"

//What we want the bot to be able to do, and how we want it to interact with the user.
//Run generated code -- NOT IMPLEMENTED YET
//Search online -- IMPLEMENTED
//Connect to Wi-Fi network -- NOT IMPLEMENTED YET, P.I.T.A.

namespace Interface {
	//CONSTANTS
	const std::string HTTPS_PORT = "443";
	const int version = 11; //HTTP version 1.1
	const std::string DuckDuckGo_REGEX = "class=\"result__snippet\"[^>]+href=\"([^ \"]+)\""; //BETA
	const int SITE_TO_ANALIZE = 3;
	//PRIVATE
	std::vector<std::string> extractUrlsFromWebPage(std::string pageContent, int count); //BETA
	std::vector<std::string> getUrlFromString(std::string string_url);
	std::vector<std::string> getUrlFromString(std::string string_url, std::vector<std::string> curUrl);

	//PUBLIC
	std::vector<std::string> getUrlFromQuery(std::string query); //BETA
	std::string sanitizePage(std::string pageContent);
	std::string getWebPage(std::vector<std::string> url);
	std::vector<std::string> retrieveDataFromRAG(RAG_Memory* rag, std::string query);
	std::vector<std::string> searchOnline(std::string query);
	std::vector<std::string> retrieveDataFromInternet(RAG_Memory* rag, std::string link, std::string query);
	std::string getActionSummary();
};
