#include "XMLParsing.h"
#include "../common/vkainfo.h"
#include <pugixml/pugixml.hpp>

#include <filesystem>
#include <cstdlib>
#include <vector>


namespace fs = std::filesystem;

namespace VKA::PARSER::XML {
	std::string XMLParsing::findXMLPath() {
		const char* sdkPathEnv = std::getenv("VULKAN_SDK");
		if (!sdkPathEnv) {
			VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), "Failed to find VULKAN_SDK! Is VULKAN_SDK installed ? ");
			return "";
		}

		fs::path sdkDir(sdkPathEnv);
		std::vector<fs::path> possiblePaths{
			sdkDir / "share" / "vulkan" / "registry" / "vk.xml",
			sdkDir / "explicit_layer.d" / ".." / "registry" / "vk.xml",
			sdkDir / "Config" / "vk.xml",
			sdkDir / "xml" / "vk.xml"
		};

		for (const auto& path : possiblePaths) {
			if (fs::exists(path)) {
				return path.string();
			}
		}

		// if possiblepaths will fail, this try-catch loop will find vk.xml with in-depth analysis
		try {
			for (const auto& entry : fs::recursive_directory_iterator(sdkDir)) {
				if (entry.is_regular_file() && entry.path().stem() == "vk" && entry.path().extension() == ".xml") {
					return entry.path().string();
				}
			}
		}
		catch (const std::exception& e) {
			VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), e.what());
		}

		VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), "VULKAN_SDK found but vk.xml was not detected within it");
		return "";
	}

	bool XMLParsing::initAndParse(VKA::DATA::GraphContext& graphcontext) {
		std::string xmlpath = findXMLPath();
		if (xmlpath.empty()) { 
			VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), "XML FILE IS EMPTY");
			return false; }

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(xmlpath.c_str());
		if (!result) {
			VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), "XML LOAD FAIL: " << result.description());
			return false;
		}

		pugi::xml_node registry = doc.child("registry");
		if (!registry) {
			VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), "There is no <registry> tag!");
			return false;
		}

		pugi::xml_node commandsnode = registry.child("commands");
		pugi::xml_node enumsnode = registry.child("enum");
		pugi::xml_node handlesnode = registry.child("handle");

		// This loop iterates over all nodes tagged with <command> within vk.xml

		for (pugi::xml_node cmdNode = commandsnode.child("command"); cmdNode; cmdNode = cmdNode.next_sibling("command")) {
			pugi::xml_node protoNode = cmdNode.child("proto");
			if (!protoNode) { continue; }

			VKA::DATA::VulkanCommandSpec spec;
			spec.name = protoNode.child_value("name");
			spec.returntype = protoNode.child_value("type");

			std::string queueAttribute = cmdNode.attribute("queues").value();
			spec.renderpassscope = cmdNode.attribute("renderpass").value();

			if (!queueAttribute.empty()) {
				size_t start = 0, end = 0;
				while ((end = queueAttribute.find(',', start)) != std::string::npos) {
					spec.queues.push_back(queueAttribute.substr(start, end - start));
					start = end + 1;
				}
				spec.queues.push_back(queueAttribute.substr(start));
			}

			// and this loop finds all parameters for commandspec
			for (pugi::xml_node paramNode = cmdNode.child("param"); paramNode; paramNode = paramNode.next_sibling("param")) {
				VKA::DATA::VulkanParamSpec param;
				param.type = paramNode.child_value("type");
				param.name = paramNode.child_value("name");

				std::string fullParamText = paramNode.text().get();
				for (pugi::xml_node child = paramNode.first_child(); child; child = child.next_sibling()) {
					fullParamText += child.value();
				}

				if (fullParamText.find('*') != std::string::npos) param.isPtr = true;
				if (fullParamText.find("const") != std::string::npos) param.isConst = true;
				if (param.type.rfind("Vk", 0) == 0) { param.isStruct = true; }
				if (fullParamText.find('[') != std::string::npos) param.isArray = true;

				spec.parameters.push_back(param);
			}
			graphcontext.commandspecs[spec.name] = spec;
		}

		// This loop iterates over all nodes tagged with <enum> within vk.xml
		for (pugi::xml_node enumnode = enumsnode.child("enum"); enumnode; enumnode = enumnode.next_sibling("enum")) {
			pugi::xml_attribute nameAttr = enumnode.attribute("name");
			if (!nameAttr) { continue; }

			std::string enumname = nameAttr.as_string();
			graphcontext.enumspecs.insert(enumname);
		}

		for (pugi::xml_node handlenode = handlesnode.child("handle"); handlenode; handlenode = handlenode.next_sibling("handle")) {
			pugi::xml_attribute nameAttr = handlenode.attribute("name");
			if (!nameAttr) { continue; }

			std::string handlename = nameAttr.as_string();
			graphcontext.typespecs.insert(handlename);
		}

		VKA_DEBUG_MSG("Vulkan Specification is loaded to memory");
		return true;
	}
} // VKA::PARSER::XML