#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace VKA::DATA {

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

	struct VkaNodeData;

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
	/////-----

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

	/////-----

	// -- XML PARSER DATA

	struct VulkanParamSpec {
		// type and name aren't same things. type is like "VkBuffer", 
		// "VkImage" and name is like "srcbuffer" and "dstbuffer" be careful!
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
	/////-----

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

		VkaNodeData& createNode(const std::string& nodeName) {
			VkaNodeData node = nodes.emplace_back();
			node.id = nextnodeid++;
			node.commandname = nodeName;

			return node;
		}

		void createLink(uint32_t startid, uint32_t endid) {
			LinkArrow link;
			link.id = nextlinkid++;
			link.startnodeid = startid;
			link.endnodeid = endid;

			links.push_back(link);
		}
	};
} // VKA::DATA