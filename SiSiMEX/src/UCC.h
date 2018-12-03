#pragma once
#include "Agent.h"

class UCC :	public Agent
{
public:

	// Constructor and destructor
	UCC(Node *node, uint16_t contributedItemId, uint16_t constraintItemId);
	~UCC();

public:

	// Agent methods
	void update() override { }
	void stop() override;
	UCC* asUCC() override { return this; }
	void OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream) override;
	bool Finished()const;

public:

	bool negotiation_result = false;
	uint16_t contributedItemId;
	uint16_t constraintItemId;

};

