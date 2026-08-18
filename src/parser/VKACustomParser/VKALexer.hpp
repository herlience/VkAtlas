#pragma once
#include <string>
#include <vector>
#include "../../common/VkAtlasData.h"

namespace VKA::PARSER::LEXER {

	class VKALexer
	{
	public:

		std::vector<VKA::DATA::Token> tokenize(const std::string& sourcecode, std::string& filepath, VKA::DATA::GraphContext context);

	private:
		std::vector<VKA::DATA::Token> tokens;
		size_t m_cursor = 0;
		std::string_view m_source;
		uint32_t line = 1;
		uint32_t column = 1;

		bool isCharSpace(char c);
		bool isDigit(char c);

		char advance();
		char peek() const;
		char peeknext() const;
		bool match(char expected);

		void addToken(VKA::DATA::Tokentype type, const std::string& text, const std::string& filepath);

		void parseVk(
			VKA::DATA::GraphContext& context,
			std::string& filepath
		);

		void parseString(std::string& filepath);

		void parseDigit(std::string& filepath);
	};

}