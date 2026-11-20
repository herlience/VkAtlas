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
		uint32_t startnpinid = 0;
		uint32_t endpinid = 0;
	};

	struct DataFromParser {
		std::vector<ASTNode> nodes;
		std::vector<LinkArrow> connections;

		std::unordered_map<std::string, Symbol> symbolmap;
		std::unordered_map<std::string, MacroDefinition> macromap;

		uint32_t totalfilesparsed = 0;
		uint32_t totallinesparsed = 0;
		std::vector<std::string> parserlogs;

		uint32_t nextpinid = 1000;

		void clear() {
			nodes.clear();
			connections.clear();
			symbolmap.clear();
			macromap.clear();
			parserlogs.clear();
			totalfilesparsed = 0;
			totallinesparsed = 0;
			nextpinid = 1000;
		}
	};

	// Stores call data extracted by the parser
	struct RawVulkanCall {
		std::string funcName;
		std::string filepath;
		int line = 0;
	};

	// -- NODE & PIN DATA

	enum class NodeStatus : uint8_t {
		Success,
		Warning, // like unresolved symbol 
		Error,
	};

	enum class PinKind : uint32_t {
		Input,
		Output
	};

	struct Pin {
		uint32_t id = 0;
		uint32_t node_id = 0;
		PinKind kind = PinKind::Input;
		std::string name;
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

		float pos_x = 0.0f;
		float pos_y = 0.0f;

		uint32_t input_pin_id = UINT32_MAX;
		uint32_t output_pin_id = UINT32_MAX;

		NodeStatus status = NodeStatus::Success;
		std::string statusMsg;
	};

	struct ASTTree {
		std::vector<ASTNode> astnodes;
		uint32_t treeid = 0;

		uint32_t add_node(ASTNode node) {
			uint32_t newid = static_cast<uint32_t>(astnodes.size() + 1);
			node.id = newid;
			astnodes.push_back(std::move(node));
			return newid;
		}
	};

	// -- XML PARSER DATA

	enum class VulkanTypeCategory : uint8_t {
		Unknown,
		External,
		Define,
		BaseType,
		Bitmask,
		Handle,
		Enum,
		FuncPointer,
		Struct,
		Union
	};

	struct VulkanVariableSpec {
		std::string name;
		std::string type;
		std::string fulltype;
		std::string declaration;
		VulkanTypeCategory category = VulkanTypeCategory::Unknown;

		bool isPtr = false;
		bool isConst = false;
		bool isStruct = false;
		bool isArray = false;
		bool isOptional = false;
		bool noautovalidity = false;

		uint32_t pointerdepth = 0;
		std::vector<std::string> arraysizes;
		std::vector<uint32_t> arraylengths;
		uint32_t bitfieldwidth = 0;

		std::vector<bool> optional;
		std::string len;
		std::string altlen;
		std::vector<std::string> lens;
		std::vector<std::string> lengthfor;

		std::string values;
		std::string externsync;
		std::string selector;
		std::string selection;
		std::string objecttype;
		std::string limittype;
		std::string validstructs;
		std::string stride;
		std::string flagsextend;
		std::string flagsextendmember;
		std::string namealias;
		std::string featurelink;
		std::string deprecated;
		std::string comment;
	};
	using VulkanParamSpec  = VulkanVariableSpec;
	using VulkanMemberSpec = VulkanVariableSpec;

	struct VulkanDeprecation {
		std::string deprecatedby;
		std::string supersededby;
		std::string explanationlink;

		bool isDeprecated() const { return !deprecatedby.empty(); }
	};

	struct VulkanSpecBase {
		std::string name;
		std::string aliasof;
		std::vector<std::string> providedby;
		VulkanDeprecation deprecation;
		std::string comment;

		bool isAlias() const { return !aliasof.empty(); }
	};

	struct VulkanStructSpec : VulkanSpecBase {
		bool isUnion = false;
		bool returnedonly = false;
		bool allowduplicate = false;
		bool requiredlimittype = false;
		std::string stype;
		std::vector<std::string> structextends;
		std::vector<std::string> extendedby;
		std::vector<VulkanMemberSpec> members;

		const VulkanMemberSpec* findMember(const std::string& memberName) const {
			for (const VulkanMemberSpec& m : members)
				if (m.name == memberName) return &m;
			return nullptr;
		}
	};

	struct VulkanHandleSpec : VulkanSpecBase {
		std::vector<std::string> parents;
		bool isDispatchable = false;
		std::string objtypeenum;
	};

	struct VulkanEnumValueSpec {
		std::string name;
		std::string aliasof;
		int64_t value = 0;
		int32_t bitpos = -1;
		std::string deprecated;
		std::string protect;
		std::vector<std::string> providedby;
		std::string comment;
	};

	struct VulkanEnumSpec : VulkanSpecBase {
		bool isBitmask = false;
		uint32_t bitwidth = 32;
		std::vector<VulkanEnumValueSpec> values;

		const VulkanEnumValueSpec* findValue(const std::string& valueName) const {
			for (const VulkanEnumValueSpec& v : values)
				if (v.name == valueName) return &v;
			return nullptr;
		}
	};

	struct VulkanBitmaskSpec : VulkanSpecBase {
		std::string flagbits;
		std::string basetype;
	};

	struct VulkanFuncPointerSpec : VulkanSpecBase {
		std::string returntype;
		std::vector<VulkanParamSpec> parameters;
	};

	struct VulkanBaseTypeSpec : VulkanSpecBase {
		std::string underlying;
		std::string definition;
	};

	struct VulkanDefineSpec : VulkanSpecBase {
		std::string definition;
	};

	struct VulkanConstantSpec : VulkanSpecBase {
		std::string type;
		std::string value;
	};

	struct VulkanCommandSpec : VulkanSpecBase {
		std::string returntype;
		std::vector<VulkanParamSpec> parameters;

		std::vector<std::string> queues;
		std::string renderpassscope;
		std::vector<std::string> cmdbufferlevel;
		std::vector<std::string> tasks;
		std::string videocoding;
		std::string conditionalrendering;
		bool allownoqueues = false;
		bool isExported = false;
		std::vector<std::string> successcodes;
		std::vector<std::string> errorcodes;
		std::vector<std::string> implicitexternsyncparams;

		const VulkanParamSpec* findParam(const std::string& paramName) const {
			for (const VulkanParamSpec& p : parameters)
				if (p.name == paramName) return &p;
			return nullptr;
		}
	};

	struct VulkanFeatureSpec {
		std::string name;
		bool isExtension = false;
		std::string number;
		std::string type;
		std::string depends;
		std::string platform;
		std::string protect;
		std::string promotedto;
		std::string deprecatedby;
		std::string obsoletedby;
		std::string specialuse;
		std::string author;
		bool isProvisional = false;
		uint32_t specversion = 0;
		std::string nameconstant;
		std::vector<std::string> types;
		std::vector<std::string> commands;
		std::vector<std::string> enums;
		std::vector<std::string> requiredfeatures;
		std::string comment;
	};

	// -- GRAPH CONTEXT 

	struct GraphContext {

		std::vector<LinkArrow> links;

		std::unordered_map<uint32_t, Pin> pin_map;

		std::unordered_map<std::string, std::vector<RawVulkanCall>> symbolmap;

		std::unordered_map<std::string, VulkanCommandSpec>     commandspecs;
		std::unordered_map<std::string, VulkanStructSpec>      structspecs;
		std::unordered_map<std::string, VulkanHandleSpec>      handlespecs;
		std::unordered_map<std::string, VulkanEnumSpec>        enumtypespecs;
		std::unordered_map<std::string, VulkanBitmaskSpec>     bitmaskspecs;
		std::unordered_map<std::string, VulkanFuncPointerSpec> funcpointerspecs;
		std::unordered_map<std::string, VulkanBaseTypeSpec>    basetypespecs;
		std::unordered_map<std::string, VulkanDefineSpec>      definespecs;
		std::unordered_map<std::string, VulkanConstantSpec>    constantspecs;
		std::unordered_map<std::string, VulkanFeatureSpec>     featurespecs;
		std::unordered_map<std::string, VulkanTypeCategory>    typecategories;
		std::unordered_map<std::string, std::string>           enumvalueowners;

		std::unordered_set<std::string> enumspecs;
		std::unordered_set<std::string> typespecs;

		std::unordered_map<std::string, std::vector<std::string>> declaredvariables;

		uint32_t nextnodeid = 1;
		uint32_t nextlinkid = 1;

		void createLink(uint32_t startid, uint32_t endid) {
			LinkArrow link;
			link.id = nextlinkid++;
			link.startnpinid = startid;
			link.endpinid = endid;

			links.push_back(link);
		}
	};
} // VKA::DATA