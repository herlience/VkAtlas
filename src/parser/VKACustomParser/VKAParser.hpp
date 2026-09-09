#pragma once
#include "../../common/VkAtlasData.h"

namespace VKA::PARSER::CUSTOM {
    class VKAParser {
    private:
        std::vector<VKA::DATA::Token> m_tokens;
        VKA::DATA::DataFromParser& m_globalData; // Veriler doðrudan buraya akacak
        size_t m_cursor = 0;
        uint32_t m_nextNodeId = 1;

        // --- GÜVENLÝ YARDIMCI FONKSÝYONLAR ---
        bool isAtEnd() const;
        VKA::DATA::Token peek() const;
        VKA::DATA::Token advance();
        bool match(VKA::DATA::Tokentype type, const std::string& text = "");
        VKA::DATA::Token consume(VKA::DATA::Tokentype type, const std::string& errMsg);
        VKA::DATA::Token consumeText(const std::string& text, const std::string& errMsg);

        // --- AYRIÞTIRICILAR ---
        void parseFunction(VKA::DATA::ASTTree& tree);
        void parseVariable();
        void parseExpression();
    public:
        // Kurucu (Constructor)
        VKAParser(std::vector<VKA::DATA::Token> tokenList, VKA::DATA::DataFromParser& globalData);

        // Ana ayrýþtýrma döngüsü
        void parse(VKA::DATA::ASTTree& tree);
    };
}

