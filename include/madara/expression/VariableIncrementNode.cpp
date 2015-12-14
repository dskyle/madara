
#ifndef _MADARA_NO_KARL_
#include "madara/expression/Visitor.h"
#include "madara/expression/LeafNode.h"
#include "madara/expression/VariableIncrementNode.h"
#include "madara/utility/Utility.h"


#include <string>
#include <sstream>

madara::expression::VariableIncrementNode::VariableIncrementNode (
  ComponentNode * lhs, madara::knowledge::KnowledgeRecord value,
  ComponentNode * rhs, 
  madara::knowledge::ThreadSafeContext &context)
: ComponentNode (context.get_logger ()), var_ (0), array_ (0),
  value_ (value), rhs_ (rhs), context_ (context)
{
  var_ = dynamic_cast <VariableNode *> (lhs);

  if (!var_)
    array_ = dynamic_cast <CompositeArrayReference *> (lhs);
}

madara::expression::VariableIncrementNode::~VariableIncrementNode ()
{
  // do not clean up record_. Let the context clean that up.
}

void
madara::expression::VariableIncrementNode::accept (Visitor &visitor) const
{
  visitor.visit (*this);
}

madara::knowledge::KnowledgeRecord
madara::expression::VariableIncrementNode::item () const
{
  knowledge::KnowledgeRecord value;

  if (var_)
    value = var_->item ();
  else if (array_)
    value = array_->item ();

  return value;
}

/// Prune the tree of unnecessary nodes. 
/// Returns evaluation of the node and sets can_change appropriately.
/// if this node can be changed, that means it shouldn't be pruned.
madara::knowledge::KnowledgeRecord
madara::expression::VariableIncrementNode::prune (bool & can_change)
{
  bool left_child_can_change = false;
  bool right_child_can_change = false;
  madara::knowledge::KnowledgeRecord right_value;

  if (this->var_ != 0 || this->array_ != 0)
    left_child_can_change = true;
  else
  {
    madara_logger_ptr_log (logger_, logger::LOG_EMERGENCY,
      "KARL COMPILE ERROR: Variable assignment has no variable\n");

    exit (-1);    
  }

  if (this->rhs_)
  {
    right_value = this->rhs_->prune (right_child_can_change);
    if (!right_child_can_change && dynamic_cast <LeafNode *> (rhs_) == 0)
    {
      delete this->rhs_;
      this->rhs_ = new LeafNode (*(this->logger_), right_value);
    }
  }
  else
  {
    madara_logger_ptr_log (logger_, logger::LOG_EMERGENCY,
      "KARL COMPILE ERROR: Assignment has no right expression\n");

    exit (-1);
  }

  can_change = left_child_can_change || right_child_can_change;

  return right_value;
}

/// Evaluates the node and its children. This does not prune any of
/// the expression tree, and is much faster than the prune function
madara::knowledge::KnowledgeRecord 
madara::expression::VariableIncrementNode::evaluate (
  const madara::knowledge::KnowledgeUpdateSettings & settings)
{
  madara::knowledge::KnowledgeRecord rhs;

  if (rhs_)
    rhs = rhs_->evaluate (settings);
  else
    rhs = value_;

  if (var_)
  {
    madara_logger_ptr_log (logger_, logger::LOG_MINOR,
      "CompositeAssignmentNode::evaluate: "
      "Attempting to set variable %s to %s.\n",
      var_->expand_key ().c_str (),
      rhs.to_string ().c_str ());
    
    knowledge::KnowledgeRecord result (var_->evaluate (settings) + rhs);
    var_->set (result, settings);
    return result;
  }
  else if (array_)
  {
    madara_logger_ptr_log (logger_, logger::LOG_MINOR,
      "CompositeAssignmentNode::evaluate: "
      "Attempting to set index of var %s to %s.\n",
      array_->expand_key ().c_str (),
      rhs.to_string ().c_str ());

    knowledge::KnowledgeRecord result (array_->evaluate (settings) + rhs);
    array_->set (result, settings);
    return result;
  }
  else
  {
    madara_logger_ptr_log (logger_, logger::LOG_MINOR,
      "CompositeAssignmentNode::evaluate: "
      "left hand side was neither a variable nor an array reference. "
      "Check your expression for errors.\n",
      array_->expand_key ().c_str (),
      rhs.to_string ().c_str ());
  }

  // return the value
  return rhs;
}

#endif // _MADARA_NO_KARL_