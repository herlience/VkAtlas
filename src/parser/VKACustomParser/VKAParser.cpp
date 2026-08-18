#include "VKAParser.hpp"
#include "../../common/vkainfo.h"

namespace VKA::PARSER::CUSTOM {
	void VKAParser::parseAST(std::vector<VKA::DATA::Token> alltokens) {
		m_tokens = std::move(alltokens);

		while (!isAtEnd()) {
			VKA::DATA::Token atoken = m_tokens[m_cursor];
			VKA_DEBUG_MSG(atoken.text);
			m_cursor++;
		}
	}
	

	// PRIVATE FUNCTIONS
	bool VKAParser::isAtEnd() {
		return m_cursor >= m_tokens.size();
	}
}