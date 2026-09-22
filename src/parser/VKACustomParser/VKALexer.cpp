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

			// 1. Boþluklar
			if (isCharSpace(c)) {
				advance();
				continue;
			}

			// 2. Yeni Satýr
			if (c == '\n') {
				line++;
				column = 1;
				m_cursor++;
				continue;
			}

			// 3. Yorum Satýrlarý ve Bölme Operatörü (CRITICAL FIX)
			if (c == '/') {
				if (peeknext() == '/') { // Tek satýrlýk yorum //
					while (m_cursor < m_source.length() && peek() != '\n') {
						advance();
					}
					continue;
				}
				else if (peeknext() == '*') { // Çok satýrlýk yorum /* ... */
					advance(); advance(); // /* karakterlerini atla
					while (m_cursor < m_source.length()) {
						if (peek() == '\n') { line++; column = 1; }
						if (peek() == '*' && peeknext() == '/') {
							advance(); advance(); // */ kapat
							break;
						}
						advance();
					}
					continue;
				}
				else {
					addToken(VKA::DATA::Tokentype::Operator, "/", filepath);
					advance();
					continue;
				}
			}

			// 4. Oklar ve Scope Operatörleri (-> ve ::)
			if (c == '-' && peeknext() == '>') {
				addToken(VKA::DATA::Tokentype::Operator, "->", filepath);
				advance(); advance();
				continue;
			}
			if (c == ':' && peeknext() == ':') {
				addToken(VKA::DATA::Tokentype::Operator, "::", filepath);
				advance(); advance();
				continue;
			}

			// 5. Tekli Operatörler
			if (c == '{' || c == '[' || c == ']' || c == '}' || c == '.' || c == '=' ||
				c == '(' || c == ')' || c == ';' || c == ',' || c == '*' || c == '+' || c == '-') {
				addToken(VKA::DATA::Tokentype::Operator, std::string(1, c), filepath);
				advance();
				continue;
			}

			// 6. Stringler
			if (c == '"') {
				parseString(filepath);
				continue;
			}

			// 7. Kelimeler / Vulkan Komutlarý
			if (isalpha(c) || c == '_') {
				parseVk(context, filepath);
				continue;
			}

			// 8. Karþýlaþtýrma Operatörleri
			if (c == '<') {
				advance();
				if (match('<')) addToken(VKA::DATA::Tokentype::Operator, "<<", filepath);
				else if (match('=')) addToken(VKA::DATA::Tokentype::Operator, "<=", filepath);
				else addToken(VKA::DATA::Tokentype::Operator, "<", filepath);
				continue;
			}

			if (c == '>') {
				advance();
				if (match('>')) addToken(VKA::DATA::Tokentype::Operator, ">>", filepath);
				else if (match('=')) addToken(VKA::DATA::Tokentype::Operator, ">=", filepath);
				else addToken(VKA::DATA::Tokentype::Operator, ">", filepath);
				continue;
			}

			if (c == '&') {
				advance();
				if (match('&')) addToken(VKA::DATA::Tokentype::Operator, "&&", filepath);
				else addToken(VKA::DATA::Tokentype::Operator, "&", filepath);
				continue;
			}

			if (c == '|') {
				advance();
				if (match('|')) addToken(VKA::DATA::Tokentype::Operator, "||", filepath);
				else addToken(VKA::DATA::Tokentype::Operator, "|", filepath);
				continue;
			}

			// 9. Makrolar (#include, #define)
			if (c == '#') {
				std::string directive;
				while (m_cursor < m_source.length()) {
					char current = peek();
					if (isCharSpace(current) || current == '\n' || current == '<' || current == '"') break;
					directive.push_back(advance());
				}
				addToken(VKA::DATA::Tokentype::Macro, directive, filepath);
				continue;
			}

			// 10. Sayýlar
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

	void VKALexer::parseVk(const VKA::DATA::GraphContext& context, const std::string& filepath) {
		std::string kelime;

		while (m_cursor < m_source.length()) {
			char c = m_source[m_cursor];
			if (c == ' ' || c == '\n' || c == '\t' || c == '\r' ||
				c == '{' || c == '}' || c == '[' || c == ']' ||
				c == ';' || c == '(' || c == ')' || c == '<' || c == '>' ||
				c == '.' || c == '=' || c == ',' || c == '*' || c == '&' ||
				c == '+' || c == '-' || c == '/' || c == ':') {
				break;
			}
			kelime.push_back(c);
			advance();
		}

		if (kelime.empty()) return;

		bool isVulkanPrefix = (kelime.size() >= 2) &&
			((kelime[0] == 'v' && kelime[1] == 'k') ||
				(kelime[0] == 'V' && kelime[1] == 'k') ||
				(kelime[0] == 'V' && kelime[1] == 'K'));

		if (isVulkanPrefix) {
			if (context.commandspecs.find(kelime) != context.commandspecs.end()) {
				addToken(VKA::DATA::Tokentype::Vkcommand, kelime, filepath);
				return;
			}
			if (context.enumspecs.find(kelime) != context.enumspecs.end()) {
				addToken(VKA::DATA::Tokentype::Vkenum, kelime, filepath);
				return;
			}
			if (context.typespecs.find(kelime) != context.typespecs.end()) {
				addToken(VKA::DATA::Tokentype::Vktype, kelime, filepath);
				return;
			}
		}

		// Standart C++ Deðiþkeni / Identifier
		addToken(VKA::DATA::Tokentype::Identifier, kelime, filepath);
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

			if (c == '\n') break;

			if (c == '\\') {
				advance();
				char next = peek();
				if (next == 'n') { strcontext.push_back('\n'); advance(); continue; }
				if (next == 't') { strcontext.push_back('\t'); advance(); continue; }
				if (next == '"') { strcontext.push_back('"'); advance(); continue; }
				if (next == '\\') { strcontext.push_back('\\'); advance(); continue; }
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
			else if ((c == 'f' || c == 'F' || c == 'u' || c == 'U') && isfloat) {
				digit.push_back(advance()); 
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