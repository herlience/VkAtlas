#pragma once 

#include <string>
#include "../core/VkAtlasData.h"

namespace VKA::PARSER {
	class XMLParsing {
	public: 
		XMLParsing() = default;
		~XMLParsing() = default;

		bool initAndParse(VKA::DATA::GraphContext& graphcontext);
	private:
		std::string findXMLPath();
	};
}