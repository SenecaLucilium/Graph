#include "ExpressionNode.h"

#include <stdexcept>
#include <utility>

namespace src::core::ExpressionEngine::Parser
{

ExpressionTree::ExpressionTree(std::unique_ptr<ExpressionNode> root_)
{
    if (root_) this->root = std::move(root_);
    else throw std::invalid_argument("Cannot create ExpressionTree with nullptr root");
}

} // namespace src::core::ExpressionEngine::Parser
