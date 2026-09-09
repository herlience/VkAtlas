#include "VKAParser.hpp"
#include "../../common/vkainfo.h"

namespace VKA::PARSER::CUSTOM {
    VKAParser::VKAParser(std::vector<VKA::DATA::Token> tokenList, VKA::DATA::DataFromParser& globalData)
        : m_tokens(std::move(tokenList)), m_globalData(globalData) {
        m_nextNodeId = m_globalData.nodes.size() + 1;
    }

    void VKAParser::parse(VKA::DATA::ASTTree& tree) {
        try
        {
            while (!isAtEnd()) {
                VKA::DATA::Token current = m_tokens[m_cursor];

                if (current.type == VKA::DATA::Tokentype::Vkcommand) {
                    parseFunction(tree);
                }
                else { advance(); }


            }
        }
        catch (const std::exception& e)
        {
            VKA_ERROR(ErrorTypeToString(ErrorType::VKA_PARSER_ERROR), "Failed to parse AST! : " << e.what());
        }
    }

    // -- PARSER FUNCTIONS 
    
    void VKAParser::parseFunction(VKA::DATA::ASTTree& tree) {
        VKA::DATA::Token current = m_tokens[m_cursor];
        VKA::DATA::ASTNode newnode;
        newnode.type = VKA::DATA::ASTNodeType::FunctionCall;
        VKA::DATA::FunctionCallNodeData calldata;
        calldata.function_name = current.text;

        std::vector<uint32_t> argument_indices;
        advance();

        while (!isAtEnd() && peek().text != ")") {
            if (peek().text == "\n" || peek().text == "\t" || peek().text == "," || peek().text == "(") { advance(); }
            else if (peek().type == VKA::DATA::Tokentype::Identifier) {
                uint32_t newindex = tree.astnodes.size();
                std::string text = peek().text;
                tree.astnodes.push_back(VKA::DATA::ASTNode{ 
                    .id = newindex, 
                    .data = VKA::DATA::ExpressionNodeData{.text = text} 
                });

                argument_indices.push_back(newindex);
                advance();
                // burada kaldým anlarsýn zaten nasýl yaptýðýmý
                // þöyle ki sonraki token identifier ise expression olarak ast node halinde yazýyorum
                // devamýný getir
            }
            else { advance(); }
        }

        if (peek().text == ")") advance();

        calldata.arguments_indices = argument_indices;
        
        newnode.data = calldata;
        newnode.id = tree.astnodes.size();

        tree.astnodes.push_back(newnode);
    }

    // -- PRIVATE FUNCTIONS
    bool VKAParser::isAtEnd() const {
        return m_cursor >= m_tokens.size() || m_tokens[m_cursor].type == VKA::DATA::Tokentype::Unknown;
    }

    VKA::DATA::Token VKAParser::peek() const {
        if (isAtEnd()) {
            return VKA::DATA::Token{ VKA::DATA::Tokentype::Unknown, "EOF", 0, 0, "" };
        }
        return m_tokens[m_cursor];
    }

    VKA::DATA::Token VKAParser::advance() {
        if (!isAtEnd()) {
            m_cursor++;
            return m_tokens[m_cursor - 1];
        }
        return VKA::DATA::Token{ VKA::DATA::Tokentype::Unknown, "EOF", 0, 0, "" };
    }

    
    bool VKAParser::match(VKA::DATA::Tokentype type, const std::string& text) {
        if (isAtEnd()) return false;

        if (peek().type == type) {
            if (text.empty() || peek().text == text) {
                advance();
                return true;
            }
        }
        return false;
    }

    VKA::DATA::Token VKAParser::consume(VKA::DATA::Tokentype type, const std::string& errMsg) {
        if (peek().type == type) return advance();
        VKA_DEBUG_MSG(errMsg + " Found: " << peek().text + " (line: " + std::to_string(peek().line) + ")");
    }

    VKA::DATA::Token VKAParser::consumeText(const std::string& text, const std::string& errMsg) {
        if (peek().text == text) return advance();
        VKA_DEBUG_MSG(errMsg + " Found: " << peek().text + " (line: " + std::to_string(peek().line) + ")");
    }

}// VKA::PARSER::CUSTOM