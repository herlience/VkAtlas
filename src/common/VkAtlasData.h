#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <memory>
#include <variant>

namespace VKA::DATA {

	// Forward Declarations
	struct VkaNodeData;
	struct ASTNode;

	// -- PARSER DATA

	enum class Tokentype : uint32_t {
		Vktype,
		Vkcommand,
		Vkenum,
		Identifier,
		Operator,
		Literal,
		StringLiteral,
		FloatLiteral,
		IntegerLiteral,
		Macro,
		Unknown
	};

	struct Token {
		Tokentype type = Tokentype::Unknown;
		std::string text;
		uint32_t line = 0;
		uint32_t column = 0;
		std::string filepath;
	};

	struct Symbol {
		std::string type;
		std::string name;
		std::string parentStruct;
		bool isPtr = false;
		bool isReferance = false;
		bool isConst = false;
		std::string scopeID;
	};

	struct MacroDefinition {
		std::string name;
		std::string replacement;
		std::string sourcefile;
		uint32_t line = 0;
	};

	struct LinkArrow {
		uint32_t id = 0;
		uint32_t startnodeid = 0;
		uint32_t endnodeid = 0;
	};

	struct DataFromParser {
		std::vector<VkaNodeData> nodes;
		std::vector<LinkArrow> connections;

		std::unordered_map<std::string, Symbol> symbolmap;
		std::unordered_map<std::string, MacroDefinition> macromap;

		uint32_t totalfilesparsed = 0;
		uint32_t totallinesparsed = 0;
		std::vector<std::string> parserlogs;

		void clear() {
			nodes.clear();
			connections.clear();
			symbolmap.clear();
			macromap.clear();
			parserlogs.clear();
			totalfilesparsed = 0;
			totallinesparsed = 0;
		}
	};

	// Stores call data extracted by the parser
	struct RawVulkanCall {
		std::string funcName;
		std::string filepath;
		int line = 0;
	};

	// -- NODE DATA

	enum class NodeStatus : uint8_t {
		Success,
		Warning, // like unresolved symbol 
		Error,
	};

	struct VkaNodeData {
		uint32_t id = 0;
		std::string commandname;
		std::string sourcefile;
		std::string parentfunction;
		uint32_t line = 0;

		std::vector<std::string> args;
		std::vector<std::string> inputRes;
		std::vector<std::string> outputRes;

		NodeStatus status = NodeStatus::Success;
		std::string statusMsg; // warning or error explanation
	};

	// -- AST DATA STRUCTS

	enum class ASTNodeType : uint32_t {
		Block,
		VariableDecl,
		FunctionCall,
		Expression
	};

	struct BlockNodeData {
		std::vector<uint32_t> statements_indices;
	};

	struct VariableDeclNodeData {
		std::string type;
		std::string name;
		uint32_t init_expression = UINT32_MAX;
	};

	struct FunctionCallNodeData {
		std::string function_name;
		std::vector<uint32_t> arguments_indices;
	};

	struct ExpressionNodeData {
		std::string text;
		bool is_address_of = false;
	};

	// -- AST NODE & TREE

	struct ASTNode {
		uint32_t id = 0;
		uint32_t line = 0;

		ASTNodeType type = ASTNodeType::Expression;

		std::variant<BlockNodeData, VariableDeclNodeData, FunctionCallNodeData, ExpressionNodeData> data;
		std::vector<std::string> inputRes;
		std::vector<std::string> outputRes;

		NodeStatus status = NodeStatus::Success;
		std::string statusMsg;
	};

	struct ASTTree {
		std::vector<ASTNode> astnodes;
		uint32_t treeid = 0;

		uint32_t add_node(ASTNode node) {
			astnodes.push_back(std::move(node));
			return static_cast<uint32_t>(astnodes.size() - 1);
		}
	};

	// -- XML PARSER DATA

	struct VulkanParamSpec {
		std::string type;
		std::string name;

		bool isPtr = false;
		bool isConst = false;
		bool isStruct = false;
		bool isArray = false;
	};

	struct VulkanCommandSpec {
		std::string name;
		std::string returntype;
		std::vector<VulkanParamSpec> parameters;

		std::vector<std::string> queues;
		std::string renderpassscope;
	};

	// -- GRAPH CONTEXT 

	struct GraphContext {

		std::vector<VkaNodeData> nodes;
		std::vector<LinkArrow> links;

		std::unordered_map<std::string, std::vector<RawVulkanCall>> symbolmap;

		std::unordered_map<std::string, VulkanCommandSpec> commandspecs;
		std::unordered_set<std::string> enumspecs;
		std::unordered_set<std::string> typespecs;

		std::unordered_map<std::string, std::vector<std::string>> declaredvariables;

		uint32_t nextnodeid = 1;
		uint32_t nextlinkid = 1;

		void createLink(uint32_t startid, uint32_t endid) {
			LinkArrow link;
			link.id = nextlinkid++;
			link.startnodeid = startid;
			link.endnodeid = endid;

			links.push_back(link);
		}
	};
} // VKA::DATA