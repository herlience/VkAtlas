#pragma once 

#include <string>
#include "../common/VkAtlasData.h"

namespace VKA::PARSER::XML {
	class XMLParsing {
	public: 
		XMLParsing() = default;
		~XMLParsing() = default;

		bool initAndParse(VKA::DATA::GraphContext& graphcontext);
	private:
		std::string findXMLPath();
	};
}