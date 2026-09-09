#pragma once
#include <string>
#include <vector>
#include "../../common/VkAtlasData.h"

namespace VKA::PARSER::LEXER {

	class VKALexer
	{
	public:

		std::vector<VKA::DATA::Token> tokenize(const std::string& sourcecode, const std::string& filepath, const VKA::DATA::GraphContext& context);

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
			const VKA::DATA::GraphContext& context,
			const std::string& filepath
		);

		void parseString(const std::string& filepath);

		void parseDigit(const std::string& filepath);
	};

}