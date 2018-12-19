#pragma once
#include "Agent.h"

// Forward declaration
class UCC;
using UCCPtr = std::shared_ptr<UCC>;

class MCC :
	public Agent
{
public:

	// Constructor and destructor
	MCC(Node *node, uint16_t contributedItemId, uint16_t constraintItemId, uint16_t _itemsNum);
	~MCC();

public:
	
	// Agent methods
	void update() override;
	void stop() override;
	MCC* asMCC() override { return this; }
	void OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream) override;

	// Getters
	bool isIdling() const;
	uint16_t contributedItemId() const { return _contributedItemId; }
	uint16_t contributedItemsNum() const { return _contributedItemsNum; }
	
	uint16_t constraintItemId() const { return _constraintItemId; }
	uint16_t constrainItemsNum() const { return _constrainItemsNum; }
	
	bool CheckInteractionAndItemsNum(uint16_t num_request);

	// Whether or not the negotiation finished
	bool negotiationFinished() const;

	bool registerIntoYellowPages();
	void unregisterFromYellowPages();
	void createChildUCC();
	void removeChildUCC();

private:

	UCCPtr _ucc; // Child UCC

	uint16_t _itemsNum; 

	uint16_t _contributedItemId; /**< The contributed item. */
	uint16_t _contributedItemsNum;

	uint16_t _constraintItemId; /**< The constraint item. */
	uint16_t _constrainItemsNum;

	
};
