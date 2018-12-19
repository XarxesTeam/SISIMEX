#pragma once

#include "Module.h"
#include "net/Net.h"
#include "Node.h"
#include "MCC.h"
#include "MCP.h"
#include <map>

class ModuleNodeCluster : public Module, public TCPNetworkManagerDelegate
{
public:

	// Virtual methods from parent class Module

	bool init() override;

	bool start() override;

	bool update() override;

	bool updateGUI() override;

	bool cleanUp() override;

	bool stop() override;

	/*
	Return true if the mcc is not in the interactions list
	*/
	bool CheckAgentInteraction(uint16_t index, uint16_t agendtTD);

	void AddNegotation(uint16_t agendtTD, uint16_t requested_item);
	void RemoveNegotiation(uint16_t agendtTD, uint16_t requested_item);

	void OnAccepted(TCPSocketPtr socket) override;
	void OnPacketReceived(TCPSocketPtr socket, InputMemoryStream &stream) override;
	void OnDisconnected(TCPSocketPtr socket) override;

private:

	bool startSystem();
	void runSystem();
	
	void spawnMCP(int nodeId, int _currentItemsNum, int requestedItemId, int requestedItemsNum, int contributedItemId, int contributedItemsNum);
	void spawnMCC(int nodeId, int contributedItemId, int constraintItemId, int contributedItemsNum);
	void spawnAgentMCC();

	std::vector<NodePtr> _nodes; /**< Array of nodes spawn in this host. */

	int state = 0; /**< State machine. */
	std::map<uint16_t, std::vector<uint16_t>> _interactions_on_work; /**< Interactions are current in process */
};
