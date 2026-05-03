#pragma once
#include <string>

//What we want the bot to be able to do, and how we want it to interact with the user.
//Run generated code -- NOT IMPLEMENTED YET
//Search online -- IMPLEMENTED
//Connect to Wi-Fi network -- NOT IMPLEMENTED YET, P.I.T.A.

namespace Interface {
	//CONSTANTS
	const std::string HTTPS_PORT = "443";
	const int version = 11; //HTTP version 1.1
	const std::string DuckDuckGo_REGEX = "class=\"result__snippet\"[^>]+href=\"([^ \"]+)\"";
	//PRIVATE
	std::vector<std::string> extractUrlsFromWebPage(std::string pageContent);
	std::vector<std::string> getUrlFromString(std::string string_url);
	//PUBLIC
	std::vector<std::string> getUrlFromQuery(std::string query);
	std::string sanitizePage(std::string pageContent);
	std::string getWebPage(std::vector<std::string> url);
	std::vector<std::string> searchOnline(std::string query);
	std::string getActionSummary();
};
