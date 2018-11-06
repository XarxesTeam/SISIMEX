#include "MCC.h"
#include "Application.h"
#include "ModuleAgentContainer.h"

// With these states you have enough so far...
enum State
{
	ST_INIT,
	ST_REGISTERING,
	ST_IDLE,
	ST_UNREGISTERING,
	ST_FINISHED
};

MCC::MCC(Node *node, uint16_t contributedItemId, uint16_t constraintItemId) :
	Agent(node),
	_contributedItemId(contributedItemId),
	_constraintItemId(constraintItemId)
{
}


MCC::~MCC()
{
}

void MCC::start()
{
	/// TODO: Set the initial state
	setState(State::ST_INIT);
}

void MCC::update()
{
	switch (state())
	{
	case ST_INIT:
		if (registerIntoYellowPages())
		{
			setState(ST_REGISTERING);
		}
		else setState(ST_FINISHED);
		break;
	case ST_REGISTERING:
		break;
	case ST_IDLE:
		break;
	case ST_UNREGISTERING:
		break;
	case ST_FINISHED:
		break;
		
		/// TODO:
		/// - Register or unregister into/from YellowPages depending on the state
		///       Use the functions registerIntoYellowPages and unregisterFromYellowPages
		///       so that this switch statement remains clean and readable
		/// - Set the next state when needed ...
	}
}

void MCC::stop()
{
	unregisterFromYellowPages();
	setState(ST_UNREGISTERING);
}


void MCC::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	// Taking the state into account, receive and deserialize packets (Ack packets) and set next state
	if (state() == ST_REGISTERING && packetHeader.packetType == PacketType::RegisterMCCAck)
	{
		/// TODO: Set the next state (Idle in this case)
		setState(State::ST_IDLE);
		/// TODO: Disconnect the socket (we don't need it anymore)
		socket->Disconnect();

	}

	/// TODO: Do the same for unregistering
	// Taking the state into account, receive and deserialize packets (Ack packets) and set next state
	if (state() == ST_UNREGISTERING && packetHeader.packetType == PacketType::UnregisterMCCAck)
	{
		/// TODO: Set the next state (Idle in this case)
		setState(State::ST_IDLE);
		/// TODO: Disconnect the socket (we don't need it anymore)
		socket->Disconnect();
	}
}

bool MCC::negotiationFinished() const
{
	return false;
}

bool MCC::negotiationAgreement() const
{
	return false;
}

bool MCC::registerIntoYellowPages()
{
	/// TODO: Create a PacketHeader (make it in Packets.h)
	PacketHeader packet_header;
	packet_header.packetType = PacketType::RegisterMCC;
	packet_header.srcAgentId = id();
	
	/// TODO: Create a PacketRegisterMCC (make it in Packets.h)
	PacketRegisterMCC packet_registerMCC;
	packet_registerMCC.itemId = _contributedItemId;

	/// TODO: Serialize both packets into an OutputMemoryStream
	OutputMemoryStream outStream;
	packet_header.Write(outStream);
	packet_registerMCC.Write(outStream);
	
	/// TODO: Send the stream (Agent::sendPacketToYellowPages)
	sendPacketToYellowPages(outStream);

	return true;
}

bool MCC::unregisterFromYellowPages()
{
	/// TODO: Create a PacketHeader (make it in Packets.h)
	PacketHeader packet_header;
	packet_header.packetType = PacketType::UnregisterMCC;
	packet_header.srcAgentId = id();

	/// TODO: Create a PacketUnregisterMCC (make it in Packets.h)
	PacketUnregisterMCC packet_unregisterMCC;
	packet_unregisterMCC.itemId = _contributedItemId;

	/// TODO: Serialize both packets into an OutputMemoryStream
	OutputMemoryStream outStream;
	packet_header.Write(outStream);
	packet_unregisterMCC.Write(outStream);

	/// TODO: Send the stream (Agent::sendPacketToYellowPages)
	sendPacketToYellowPages(outStream);

	return true;
}
