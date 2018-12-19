#pragma once
#include "Agent.h"

// Forward declaration
class MCP;
using MCPPtr = std::shared_ptr<MCP>;

class UCP :
	public Agent
{
public:

	// Constructor and destructor
	UCP(Node *node, uint16_t _currentItemsNum, uint16_t requestedItemId, uint16_t requestedItemsNum, uint16_t contributedItemId, uint16_t contributedItemsNum, const AgentLocation &uccLoc, unsigned int searchDepth);
	~UCP();

public:

	// Agent methods
	void update() override;
	void stop() override;
	UCP* asUCP() override { return this; }
	void OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream) override;
	
	void createChildMCP(uint16_t request, uint16_t requestedItemsNum, uint16_t contributedItemsNum);
	void removeChildMCP();
	
	bool Finished()const;

public:

	MCPPtr _mcp; //Child MCP
	AgentLocation uccAgent;
	
	uint16_t _requestedItemId;
	uint16_t _requestedItemsNum;
	uint16_t _contributedItemId;
	uint16_t _contributedItemsNum;
	
	uint16_t _currentItemsNum;
	
	int depth = 0;
	bool negotiation_result = false;
};

