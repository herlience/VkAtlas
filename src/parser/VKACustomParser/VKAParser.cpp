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
                else if (current.type == VKA::DATA::Tokentype::Vktype || current.type == VKA::DATA::Tokentype::Identifier) {
                    parseVariable(tree);
                }
                else {
                    advance();
                }
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
            if (peek().text == "\n" || peek().text == "\t" || peek().text == "," || peek().text == "(") {
                advance();
            }
            
            else if (peek().type == VKA::DATA::Tokentype::Identifier ||
                peek().type == VKA::DATA::Tokentype::Literal ||
                peek().type == VKA::DATA::Tokentype::StringLiteral ||
                peek().type == VKA::DATA::Tokentype::IntegerLiteral ||
                peek().type == VKA::DATA::Tokentype::FloatLiteral) {

                uint32_t newindex = tree.astnodes.size();
                std::string text = peek().text;

                tree.astnodes.push_back(VKA::DATA::ASTNode{
                    .id = newindex,
                    .data = VKA::DATA::ExpressionNodeData{.text = text}
                    });

                argument_indices.push_back(newindex);
                advance();
            }
            else { advance(); }
        }

        if (!isAtEnd() && peek().text == ")") advance(); 

        calldata.arguments_indices = argument_indices;
        newnode.data = calldata;
        newnode.id = tree.astnodes.size();

        tree.astnodes.push_back(newnode);
    }

    void VKAParser::parseVariable(VKA::DATA::ASTTree& tree) {
        std::string var_type = peek().text;
        advance();

        std::string var_name = "";
        if (!isAtEnd() && peek().type == VKA::DATA::Tokentype::Identifier) {
            var_name = peek().text;
            advance();
        }

        uint32_t init_expr_index = UINT32_MAX;

        if (!isAtEnd() && peek().text == "=") {
            advance(); 

            while (!isAtEnd() && (peek().text == " " || peek().text == "\t")) {
                advance();
            }

            if (!isAtEnd() && (
                peek().type == VKA::DATA::Tokentype::Literal ||
                peek().type == VKA::DATA::Tokentype::StringLiteral ||
                peek().type == VKA::DATA::Tokentype::IntegerLiteral ||
                peek().type == VKA::DATA::Tokentype::FloatLiteral ||
                peek().type == VKA::DATA::Tokentype::Identifier)) {

                init_expr_index = static_cast<uint32_t>(tree.astnodes.size());

                VKA::DATA::ASTNode exprNode;
                exprNode.id = init_expr_index;
                exprNode.type = VKA::DATA::ASTNodeType::Expression;
                exprNode.data = VKA::DATA::ExpressionNodeData{ .text = peek().text };

                tree.astnodes.push_back(exprNode);
                advance(); 
            }
        }

        while (!isAtEnd() && peek().text != ";") {
            advance();
        }
        if (!isAtEnd() && peek().text == ";") {
            advance();
        }

        uint32_t varNodeIndex = static_cast<uint32_t>(tree.astnodes.size());

        VKA::DATA::ASTNode varNode;
        varNode.id = varNodeIndex;
        varNode.type = VKA::DATA::ASTNodeType::VariableDecl;

        VKA::DATA::VariableDeclNodeData declData;
        declData.type = var_type;
        declData.name = var_name;
        declData.init_expression = init_expr_index;

        varNode.data = declData;
        tree.astnodes.push_back(varNode);
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
        return VKA::DATA::Token{ VKA::DATA::Tokentype::Unknown, "ERROR", 0, 0, "" };
    }

    VKA::DATA::Token VKAParser::consumeText(const std::string& text, const std::string& errMsg) {
        if (peek().text == text) return advance();
        VKA_DEBUG_MSG(errMsg + " Found: " << peek().text + " (line: " + std::to_string(peek().line) + ")");
        return VKA::DATA::Token{ VKA::DATA::Tokentype::Unknown, "ERROR", 0, 0, "" };
    }

}// VKA::PARSER::CUSTOM