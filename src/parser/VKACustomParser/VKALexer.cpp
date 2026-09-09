#include "VKALexer.hpp"
#include <array>
#include <algorithm>

namespace VKA::PARSER::LEXER {
	std::vector<VKA::DATA::Token> VKALexer::tokenize(const std::string& sourcecode, const std::string& filepath, const VKA::DATA::GraphContext& context) {
		m_source = sourcecode;
		m_cursor = 0;
		line = 1;
		column = 1;
		tokens.clear();

		while (m_cursor < m_source.length()) {
			char c = m_source[m_cursor];

			if (isCharSpace(c)) {
				advance();
				continue;
			}

			if (c == '\n') {
				line++;
				column = 1;
				m_cursor++;
				continue;
			}

			if (c == '{' || c == '[' || c == ']' || c == '}' || c == '.' || c == '=' ||
				c == '(' || c == ')' || c == ';' || c == ',' || c == '*') {
				addToken(VKA::DATA::Tokentype::Operator, std::string(1, c), filepath);
				advance();
				continue;
			}

			if (c == '"') {
				parseString(filepath);
				continue;
			}

			if (isalpha(c) || c == '_') {
				parseVk(context, filepath);
				continue;
			}

			if (c == '<') {
				advance();
				if (match('<')) {
					addToken(VKA::DATA::Tokentype::Operator, "<<", filepath);
				}
				else if (match('=')) {
					addToken(VKA::DATA::Tokentype::Operator, "<=", filepath);
				}
				else {
					addToken(VKA::DATA::Tokentype::Operator, "<", filepath);
				}
				continue;
			}

			if (c == '>') {
				advance();
				if (match('>')) {
					addToken(VKA::DATA::Tokentype::Operator, ">>", filepath);
				}
				else if (match('=')) {
					addToken(VKA::DATA::Tokentype::Operator, ">=", filepath);
				}
				else {
					addToken(VKA::DATA::Tokentype::Operator, ">", filepath);
				}
				continue;
			}

			if (c == '&') {
				advance();
				if (match('&')) {
					addToken(VKA::DATA::Tokentype::Operator, "&&", filepath);
				}
				else {
					addToken(VKA::DATA::Tokentype::Operator, "&", filepath);
				}
				continue;
			}

			if (c == '|') {
				advance();
				if (match('|')) {
					addToken(VKA::DATA::Tokentype::Operator, "||", filepath);
				}
				else {
					addToken(VKA::DATA::Tokentype::Operator, "|", filepath);
				}
				continue;
			}

			if (c == '#') {
				std::string directive;

				while (m_cursor < m_source.length()) {
					char current = peek();
					if (isCharSpace(current) || current == '\n' || current == '<' || current == '"') {
						break;
					}
					directive.push_back(advance());
				}

				addToken(VKA::DATA::Tokentype::Macro, directive, filepath);
				continue;
			}

			if (isDigit(c)) {
				parseDigit(filepath);
				continue;
			}

			advance();
		}

		return tokens;
	}

	// PRIVATE FUNCTIONS

	bool VKALexer::isCharSpace(char c) {
		return (c == ' ' || c == '\t' || c == '\r');
	}

	bool VKALexer::isDigit(char c) {
		return (c >= '0' && c <= '9');
	}

	char VKALexer::advance() {
		char c = m_source[m_cursor];
		m_cursor++;
		column++;
		return c;
	}

	char VKALexer::peek() const {
		if (m_cursor >= m_source.length()) return '\0';
		return m_source[m_cursor];
	}

	char VKALexer::peeknext() const {
		if (m_cursor + 1 >= m_source.length()) return '\0';
		return m_source[m_cursor + 1];
	}

	bool VKALexer::match(char expected) {
		if (m_cursor >= m_source.length()) return false;
		if (m_source[m_cursor] != expected) return false;

		m_cursor++;
		column++;
		return true;
	}

	void VKALexer::addToken(VKA::DATA::Tokentype type, const std::string& text, const std::string& filepath) {
		VKA::DATA::Token t;
		t.type = type;
		t.filepath = filepath;
		t.text = text;
		t.line = line;
		t.column = column;

		tokens.push_back(t);
	}

	void VKALexer::parseVk(
		const VKA::DATA::GraphContext& context,
		const std::string& filepath
	) {
		std::string kelime;

		while (m_cursor < m_source.length()) {
			char c = m_source[m_cursor];
			if (c == ' ' || c == '\n' || c == '\t' || c == '\r' ||
				c == '{' || c == '}' || c == '[' || c == ']' ||
				c == ';' || c == '(' || c == ')' || c == '<' || c == '>' ||
				c == '.' || c == '=' || c == ',' || c == '*' || c == '&') {
				break;
			}
			kelime.push_back(c);
			advance();
		}

		if (kelime.empty()) {
			return;
		}

		if (context.commandspecs.find(kelime) != context.commandspecs.end()) {
			addToken(VKA::DATA::Tokentype::Vkcommand, kelime, filepath);
		}
		else if (context.enumspecs.find(kelime) != context.enumspecs.end()) {
			addToken(VKA::DATA::Tokentype::Vkenum, kelime, filepath);
		}
		else if (context.typespecs.find(kelime) != context.typespecs.end()) {
			addToken(VKA::DATA::Tokentype::Vktype, kelime, filepath);
		}
		else {
			addToken(VKA::DATA::Tokentype::Identifier, kelime, filepath);
		}
	}

	void VKALexer::parseString(const std::string& filepath) {
		advance();
		std::string strcontext;

		while (m_cursor < m_source.length()) {
			char c = peek();

			if (c == '"') {
				advance();
				break;
			}

			if (c == '\n') {
				break;
			}

			if (c == '\\' && peeknext() == '"') {
				advance();
				strcontext.push_back(advance());
				continue;
			}

			strcontext.push_back(advance());
		}

		addToken(VKA::DATA::Tokentype::StringLiteral, strcontext, filepath);
	}

	void VKALexer::parseDigit(const std::string& filepath) {
		std::string digit;

		bool isfloat = false;
		while (m_cursor < m_source.length()) {
			char c = peek();

			if (isDigit(c)) {
				digit.push_back(advance());
			}
			else if (c == '.' && !isfloat) {
				isfloat = true;
				digit.push_back(advance());
			}
			else if ((c == 'f' || c == 'F') && isfloat) {
				advance();
				break;
			}
			else {
				break;
			}
		}

		if (isfloat) {
			addToken(VKA::DATA::Tokentype::FloatLiteral, digit, filepath);
		}
		else {
			addToken(VKA::DATA::Tokentype::IntegerLiteral, digit, filepath);
		}
	}
}