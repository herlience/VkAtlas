#include "VKAParser.hpp"
#include "../../common/vkainfo.h"

namespace VKA::PARSER::CUSTOM {
    VKAParser::VKAParser(std::vector<VKA::DATA::Token> tokenList, VKA::DATA::DataFromParser& globalData)
        : m_tokens(std::move(tokenList)), m_globalData(globalData) {
        m_nextNodeId = m_globalData.nodes.size() + 1;
    }

    void VKAParser::parse(VKA::DATA::ASTTree& tree) {
        try {
            while (!isAtEnd()) {
                VKA::DATA::Token current = peek();

                if (current.type == VKA::DATA::Tokentype::Vkcommand) {
                    parseFunction(tree);
                }
                else if (current.type == VKA::DATA::Tokentype::Vktype || current.type == VKA::DATA::Tokentype::Identifier) {
                    if (m_cursor + 1 < m_tokens.size() && m_tokens[m_cursor + 1].text == "(") {
                        parseFunction(tree);
                    }
                    else {
                        parseVariable(tree);
                    }
                }
                else {
                    advance();
                }
            }
        }
        catch (const std::exception& e) {
            VKA_ERROR(ErrorTypeToString(ErrorType::VKA_PARSER_ERROR), "Failed to parse AST! : " << e.what());
        }
    }

    // -- PARSER FUNCTIONS 
    
    void VKAParser::parseFunction(VKA::DATA::ASTTree& tree) {
        VKA::DATA::Token funcToken = advance(); 

        VKA::DATA::ASTNode funcNode;
        funcNode.type = VKA::DATA::ASTNodeType::FunctionCall;
        funcNode.input_pin_id = m_globalData.nextpinid++;
        funcNode.output_pin_id = m_globalData.nextpinid++;

        VKA::DATA::FunctionCallNodeData calldata;
        calldata.function_name = funcToken.text;

        if (peek().text == "(") advance();

        std::vector<uint32_t> argument_indices;

        while (!isAtEnd() && peek().text != ")") {
            if (peek().text == "," || peek().text == " " || peek().text == "\t") {
                advance();
                continue;
            }

            uint32_t argIndex = static_cast<uint32_t>(tree.astnodes.size());

            VKA::DATA::ASTNode argNode;
            argNode.id = argIndex;
            argNode.type = VKA::DATA::ASTNodeType::Expression; 
            argNode.input_pin_id = m_globalData.nextpinid++;
            argNode.output_pin_id = m_globalData.nextpinid++;
            argNode.data = VKA::DATA::ExpressionNodeData{ .text = peek().text };

            tree.astnodes.push_back(argNode);
            argument_indices.push_back(argIndex);

            advance();
        }

        if (!isAtEnd() && peek().text == ")") advance(); 

        calldata.arguments_indices = argument_indices;
        funcNode.data = calldata;
        funcNode.id = static_cast<uint32_t>(tree.astnodes.size());

        tree.astnodes.push_back(funcNode);
    }

    void VKAParser::parseVariable(VKA::DATA::ASTTree& tree) {
        std::string var_type = advance().text; 

        std::string var_name = "";
        if (!isAtEnd() && peek().type == VKA::DATA::Tokentype::Identifier) {
            var_name = advance().text;
        }

        uint32_t init_expr_index = UINT32_MAX;

        if (!isAtEnd() && peek().text == "=") {
            advance(); 
            if (peek().type == VKA::DATA::Tokentype::Vkcommand) {
                init_expr_index = static_cast<uint32_t>(tree.astnodes.size());
                parseFunction(tree); 
            }
            else if (!isAtEnd()) {
                init_expr_index = static_cast<uint32_t>(tree.astnodes.size());

                VKA::DATA::ASTNode exprNode;
                exprNode.id = init_expr_index;
                exprNode.type = VKA::DATA::ASTNodeType::Expression;
                exprNode.input_pin_id = m_globalData.nextpinid++;
                exprNode.output_pin_id = m_globalData.nextpinid++;
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

        VKA::DATA::ASTNode varNode;
        varNode.id = static_cast<uint32_t>(tree.astnodes.size());
        varNode.type = VKA::DATA::ASTNodeType::VariableDecl;
        varNode.input_pin_id = m_globalData.nextpinid++;
        varNode.output_pin_id = m_globalData.nextpinid++;

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

    void VKAParser::flatasttree(uint32_t nodeid, uint32_t parentoutpin, VKA::DATA::ASTTree& tree, VKA::DATA::GraphContext& ctx) {
        // O(1) 
        VKA::DATA::ASTNode* nodePtr = nullptr;
        for (auto& n : tree.astnodes) {
            if (n.id == nodeid) {
                nodePtr = &n;
                break;
            }
        }
        if (!nodePtr) return;
        const auto& node = *nodePtr;

        if (parentoutpin != UINT32_MAX && node.input_pin_id != UINT32_MAX) {
            ctx.createLink(parentoutpin, node.input_pin_id);
        }

        switch (node.type) {
        case VKA::DATA::ASTNodeType::FunctionCall: {
            const auto& funcData = std::get<VKA::DATA::FunctionCallNodeData>(node.data);
            for (auto argNodeId : funcData.arguments_indices) {
                flatasttree(argNodeId, node.output_pin_id, tree, ctx);
            }
            break;
        }
        case VKA::DATA::ASTNodeType::VariableDecl: {
            const auto& varData = std::get<VKA::DATA::VariableDeclNodeData>(node.data);
            if (varData.init_expression != UINT32_MAX) {
                flatasttree(varData.init_expression, node.output_pin_id, tree, ctx);
            }
            break;
        }
        case VKA::DATA::ASTNodeType::Block: {
            const auto& blockData = std::get<VKA::DATA::BlockNodeData>(node.data);
            for (auto stmtId : blockData.statements_indices) {
                flatasttree(stmtId, node.output_pin_id, tree, ctx);
            }
            break;
        }
        default:
            break;
        }
    }

}// VKA::PARSER::CUSTOM