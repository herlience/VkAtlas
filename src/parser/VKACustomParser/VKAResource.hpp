#pragma once

#include <vector>
#include "../../common/VkAtlasData.h"
namespace VKA::PARSER::RESOURCE {
	class VKAResource
	{
	public:
		std::vector<VKA::DATA::ASTNode> variabletable;

		std::vector<VKA::DATA::ASTNode> collectVariables(VKA::DATA::ASTTree tree);
	private:
		size_t cursor = 0;
		std::vector<VKA::DATA::ASTNode> source;

		VKA::DATA::ASTNode advance();
	};
}

