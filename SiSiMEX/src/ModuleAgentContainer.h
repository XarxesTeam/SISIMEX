#pragma once

#include "Module.h"
#include <memory>
#include <vector>

class Node;
class Agent;
class AgentLocation;
class MCC;
class MCP;
using AgentPtr = std::shared_ptr<Agent>;
using MCCPtr = std::shared_ptr<MCC>;
using MCPPtr = std::shared_ptr<MCP>;

class ModuleAgentContainer : public Module
{
public:

	// Constructor and destructor
	ModuleAgentContainer();
	~ModuleAgentContainer();

	// Agent creation methods
	MCCPtr createMCC(Node *node, uint16_t contributedItemId, uint16_t constraintItemId);
	MCPPtr createMCP(Node *node, uint16_t petitionedItemId, uint16_t contributedItemId);

	// Getters
	AgentPtr getAgent(int agentId);
	bool empty() const;

	// Update
	bool update() override;

	// Post update
	bool postUpdate() override;

	// Tell all agents to stop
	bool stop() override;

	// Remove all agents from memory
	bool cleanUp() override;

private:

	// Setters
	void addAgent(AgentPtr agent);

	std::vector<AgentPtr> _agentsToAdd; /**< Agents to add. */
	std::vector<AgentPtr> _agents; /**< Array of agents. */
};