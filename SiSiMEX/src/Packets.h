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
	UnregisterMCCAck,
	// MCP <-> YP
	QueryMCCsForItem,
	ReturnMCCsForItem,
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


// MCC <-> YP

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


// MCP <-> YP

/**
 * PacketQueryMCCsForItem
 * The information is the same required for PacketRegisterMCC so...
 */
// TODO
class PacketQueryMCCsForItem
{
public:
	uint16_t itemId;
	void Read(InputMemoryStream &stream) 
	{
		stream.Read(itemId);
	}
	void Write(OutputMemoryStream &stream) 
	{
		stream.Write(itemId);
	}
};

/**
 * class PacketReturnMCCsForItem
 * This packet is the response for PacketQueryMCCsForItem and
 * is sent by an MCP (MultiCastPetitioner) agent.
 * It contains a list of the addresses of MCC agents contributing
 * with the item specified by the PacketQueryMCCsForItem.
 */
// TODO
class PacketReturnMCCsForItem
{
public:
	int mccRegisterIndex;
	uint16_t mccRequestedItemId;
	uint16_t mccContributedItemId;
	std::vector<AgentLocation> mccRegisters;

	void Read(InputMemoryStream &stream)
	{
		stream.Read(mccRegisterIndex);
		stream.Read(mccRequestedItemId);
		stream.Read(mccContributedItemId);
		int size = 0;
		stream.Read(size);
		for (int i = 0; i < size; i++)
		{
			AgentLocation newAgent;
			mccRegisters.push_back(newAgent);
			mccRegisters[i].Read(stream);
		}
	}
	void Write(OutputMemoryStream &stream)
	{
		stream.Write(mccRegisterIndex);
		stream.Write(mccRequestedItemId);
		stream.Write(mccContributedItemId);
		stream.Write((int)mccRegisters.size());
		for (int i = 0; i < mccRegisters.size(); i++)
			mccRegisters[i].Write(stream);
	}
};
