#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace VKADATA {

	// Stores call data extracted by the parser
	struct RawVulkanCall {
		std::string funcName;
		std::string filepath;
		int line = 0;
	};

	 // -- NODE DATA

	enum class NodeSeverity {
		Normal = 0,
		Warning = 1,
		Error = 2
	};

	struct Node {
		uint32_t id = 0;
		std::string name;
		std::string wrappercontext;

		std::string filepath;
		int line = 0;

		std::vector<std::string> args;

		NodeSeverity severity = NodeSeverity::Normal;
		std::string warningmsg;
		std::string errormsg;
	};

	struct LinkArrow {
		uint32_t id = 0;
		uint32_t startnodeid = 0;
		uint32_t endnodeid = 0;
	};

	// -- GRAPH CONTEXT 

	struct GraphContext {

		std::vector<Node> nodes;
		std::vector<LinkArrow> links;

		std::unordered_map<std::string, std::vector<RawVulkanCall>> symbolmap;

		uint32_t nextnodeid = 1;
		uint32_t nextlinkid = 1;

		Node& createNode(const std::string& nodeName) {
			nodes.emplace_back();

			Node node = nodes.back();
			node.id = nextnodeid++;
			node.name = nodeName;

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
} // VKADATA