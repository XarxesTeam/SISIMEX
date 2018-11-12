#pragma once
#include "Agent.h"

class MCP :
	public Agent
{
public:

	// Constructor and destructor
	MCP(Node *node, uint16_t requestedItemID, uint16_t contributedItemID);
	~MCP();

	// Agent methods
	void update() override;
	void stop() override;
	void OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream) override;

	// Getters
	uint16_t requestedItemId() const { return _requestedItemId; }
	uint16_t contributedItemId() const { return _contributedItemId; }

	// Whether or not the negotiation finished
	bool negotiationFinished() const;

	// Whether or not there was a negotiation agreement
	bool negotiationAgreement() const;

private:

	bool queryMCCsForItem(int itemId);

	uint16_t _requestedItemId;
	uint16_t _contributedItemId;

	int _mccRegisterIndex; /**< Iterator through _mccRegisters. */
	std::vector<AgentLocation> _mccRegisters; /**< MCCs returned by the YP. */

	bool _negotiationAgreement; /**< Was there a negotiation agreement? */
};

