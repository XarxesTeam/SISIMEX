#pragma once
#include "Globals.h"
#include "AgentLocation.h"

/**
 * Enumerated type for packets.
 * There must be a value for each kind of packet
 * containing extra data besides the Header.
 */
enum class PacketType
{
	// MCC <-> YP
	RegisterMCC,
	RegisterMCCAck,
	UnregisterMCC,

	// MCP <-> YP
	QueryMCCsForItem,
	ReturnMCCsForItem,

	// MCP <-> MCC
	NegotiationMCPPetition,
	MCCNegotiationResponse,

	// UCP <-> UCC
	UCPItemRequest,
	UCPConstrainRequest,
	UCCConstrainRequestConclusion,
	UCCNegotiationConclusion,

	Last
};

/**
 * Standard information used by almost all messages in the system.
 * Agents will be communicating among each other, so in many cases,
 * besides the packet type, a header containing the source and the
 * destination agents involved is needed.
 */
class PacketHeader {
public:
	PacketType packetType; // Which type is this packet
	uint16_t srcAgentId;   // Which agent sent this packet?
	uint16_t dstAgentId;   // Which agent is expected to receive the packet?
	PacketHeader() :
		packetType(PacketType::Last),
		srcAgentId(NULL_AGENT_ID),
		dstAgentId(NULL_AGENT_ID)
	{ }
	void Read(InputMemoryStream &stream) {
		stream.Read(packetType);
		stream.Read(srcAgentId);
		stream.Read(dstAgentId);
	}
	void Write(OutputMemoryStream &stream) {
		stream.Write(packetType);
		stream.Write(srcAgentId);
		stream.Write(dstAgentId);
	}
};

/**
 * To register a MCC we need to know which resource/item is
 * being provided by the MCC agent.
 */
class PacketRegisterMCC {
public:
	uint16_t itemId; // Which item has to be registered?
	void Read(InputMemoryStream &stream) {
		stream.Read(itemId);
	}
	void Write(OutputMemoryStream &stream) {
		stream.Write(itemId);
	}
};

/**
* The information is the same required for PacketRegisterMCC so...
*/
using PacketUnregisterMCC = PacketRegisterMCC;

/**
* The information is the same required for PacketRegisterMCC so...
*/
using PacketQueryMCCsForItem = PacketRegisterMCC;

/**
 * This packet is the response for PacketQueryMCCsForItem and
 * is sent by an MCP (MultiCastPetitioner) agent.
 * It contains a list of the addresses of MCC agents contributing
 * with the item specified by the PacketQueryMCCsForItem.
 */
class PacketReturnMCCsForItem {
public:
	std::vector<AgentLocation> mccAddresses;
	void Read(InputMemoryStream &stream) {
		uint16_t count;
		stream.Read(count);
		mccAddresses.resize(count);
		for (auto &mccAddress : mccAddresses) {
			mccAddress.Read(stream);
		}
	}
	void Write(OutputMemoryStream &stream) {
		auto count = static_cast<uint16_t>(mccAddresses.size());
		stream.Write(count);
		for (auto &mccAddress : mccAddresses) {
			mccAddress.Write(stream);
		}
	}
};



// MCP <-> MCC
class PacketMCCNegotiationResponse
{
public:
	bool accepted;
	AgentLocation uccAgent;

	void Read(InputMemoryStream &stream)
	{
		stream.Read(accepted);
		uccAgent.Read(stream);
	}
	void Write(OutputMemoryStream &stream)
	{
		stream.Write(accepted);
		uccAgent.Write(stream);
	}
};
class PacketMCPNegotiationPetitionItemsNum 
{
public:

	unsigned int itemsNum;

	void Read(InputMemoryStream &stream) {
		stream.Read(itemsNum);

	}
	void Write(OutputMemoryStream &stream) {
		stream.Write(itemsNum);
	}
};

// UCP <-> UCC
using PacketUCPItemRequest = PacketRegisterMCC; //Used to create a class with the same content
using PacketUCPConstrainRequest = PacketRegisterMCC;

class PacketUCPConstrainRequestConclusion
{
public:

	bool negotiation_result = false;
	unsigned int constrain_num;
	unsigned int contributed_num;

	void Read(InputMemoryStream &stream)
	{
		stream.Read(negotiation_result);
		stream.Read(constrain_num);
		stream.Read(contributed_num);
	}
	void Write(OutputMemoryStream &stream)
	{
		stream.Write(negotiation_result);
		stream.Write(constrain_num);
		stream.Write(contributed_num);

	}
};

using PacketNegotiationConclusion = PacketUCPConstrainRequestConclusion;