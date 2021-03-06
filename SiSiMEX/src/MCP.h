#pragma once
#include "Agent.h"

// Forward declaration
class UCP;
using UCPPtr = std::shared_ptr<UCP>;

class MCP :
	public Agent
{
public:

	// Constructor and destructor
	MCP(Node *node, uint16_t requestedItemID, uint16_t contributedItemID, unsigned int searchDepth);
	MCP(Node *node, uint16_t _currentItemsNum, uint16_t requestedItemID, uint16_t requestedItemsNum, uint16_t contributedItemID, uint16_t contributedItemsNum, unsigned int searchDepth);

	~MCP();

	// Agent methods
	void update() override;
	void stop() override;
	MCP* asMCP() override { return this; }
	void OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream) override;

	// Getters
	uint16_t requestedItemId() const { return _requestedItemId; }
	uint16_t contributedItemId() const { return _contributedItemId; }
	uint16_t requestedItemsNum() const { return _requestedItemsNum; }
	uint16_t contributedItemsNum() const { return _contributedItemsNum; }

	// Whether or not the negotiation finished
	bool negotiationFinished() const;

	// Whether or not there was a negotiation agreement
	bool negotiationAgreement() const;

	// It returns the search depth of this MCP
	unsigned int searchDepth() const { return _searchDepth; }

private:

	bool queryMCCsForItem(int itemId);

	void createChildUCP(AgentLocation &uccAgent);
	void removeChildUCP();

private:

	int _mccRegisterIndex = 0; /**< Iterator through _mccRegisters. */
	std::vector<AgentLocation> _mccRegisters; /**< MCCs returned by the YP. */

	UCPPtr _ucp; //Child UCP
	unsigned int _searchDepth = 0;
	bool negotiation_result = false;

	uint16_t _requestedItemId;
	uint16_t _requestedItemsNum = 1;

	uint16_t _contributedItemId;
	uint16_t _contributedItemsNum = 1;

	uint16_t __currentItemsNum = 1;
};

