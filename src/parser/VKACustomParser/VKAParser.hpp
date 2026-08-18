#pragma once
#include "../../common/VkAtlasData.h"

namespace VKA::PARSER::CUSTOM {
	class VKAParser
	{
	public:

		void parseAST(std::vector<VKA::DATA::Token> alltokens);
	private:
		std::vector<VKA::DATA::Token> m_tokens;
		size_t m_cursor = 0;

		bool isAtEnd();
	};
}

