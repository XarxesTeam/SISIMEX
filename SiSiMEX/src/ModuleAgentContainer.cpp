#include "ModuleAgentContainer.h"
#include "MCC.h"
#include "MCP.h"


ModuleAgentContainer::ModuleAgentContainer()
{
}

ModuleAgentContainer::~ModuleAgentContainer()
{
}

MCCPtr ModuleAgentContainer::createMCC(Node *node, uint16_t contributedItemId, uint16_t constraintItemId)
{
	MCCPtr mcc(new MCC(node, contributedItemId, constraintItemId));
	addAgent(mcc);
	return mcc;
}

MCPPtr ModuleAgentContainer::createMCP(Node *node, uint16_t petitionedItemId, uint16_t contributedItemId)
{
	MCPPtr mcp(new MCP(node, petitionedItemId, contributedItemId));
	addAgent(mcp);
	return mcp;
}

void ModuleAgentContainer::addAgent(AgentPtr agent)
{
	_agentsToAdd.push_back(agent);
}

AgentPtr ModuleAgentContainer::getAgent(int agentId)
{
	// Agent search
	for (auto agent : _agents) {
		if (agent->id() == agentId) {
			return agent;
		}
	}

	return nullptr;
}

bool ModuleAgentContainer::empty() const
{
	return _agents.empty();
}

bool  ModuleAgentContainer::update()
{
	// Update all agents
	for (auto agent : _agents)
	{
		if (agent->isValid()) {
			agent->update();
		}
	}

	return true;
}

bool ModuleAgentContainer::postUpdate()
{
	// Add pending agents to add
	for (auto agentToAdd : _agentsToAdd)
	{
		_agents.push_back(agentToAdd);
	}
	_agentsToAdd.clear();

	// Track alive agents
	std::vector<AgentPtr> agentsAlive;

	// Update all agents
	for (auto agent : _agents)
	{
		// Keep track of alive agents
		if (agent->isValid()) {
			agentsAlive.push_back(agent);
		}
	}

	// Remove finished agents
	_agents.swap(agentsAlive);

	return true;
}

bool ModuleAgentContainer::stop()
{
	// Finalize all agents
	for (auto agent : _agents) {
		agent->stop();
	}

	return true;
}

bool ModuleAgentContainer::cleanUp()
{
	_agents.clear();

	return true;
}
