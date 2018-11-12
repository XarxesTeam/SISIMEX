#include "MCC.h"
#include "Application.h"
#include "ModuleAgentContainer.h"

enum State
{
	ST_INIT,
	ST_REGISTERING,
	ST_IDLE,
	ST_NEGOTIATING,
	ST_NEGOTIATION_FINISHED,
	ST_UNREGISTERING,
	ST_FINISHED
};

MCC::MCC(Node *node, uint16_t contributedItemId, uint16_t constraintItemId) :
	Agent(node),
	_contributedItemId(contributedItemId),
	_constraintItemId(constraintItemId)
{
	setState(ST_INIT);
}


MCC::~MCC()
{
}

void MCC::update()
{
	switch (state())
	{
	case ST_INIT:
		if (registerIntoYellowPages()) {
			setState(ST_REGISTERING);
		} else {
			setState(ST_FINISHED);
		}
		break;
	case ST_REGISTERING:
		// See OnPacketReceived()
		break;
	case ST_UNREGISTERING:
		// See OnPacketReceived()
		break;
	case ST_FINISHED:
		destroy();
	}
}

void MCC::stop()
{
	unregisterFromYellowPages();
	setState(ST_UNREGISTERING);
}


void MCC::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	const PacketType packetType = packetHeader.packetType;
	if (state() == ST_REGISTERING && packetType == PacketType::RegisterMCCAck) {
		setState(ST_IDLE);
		socket->Disconnect();
	}
	else if (state() == ST_UNREGISTERING && packetType == PacketType::UnregisterMCCAck) {
		setState(ST_FINISHED);
		socket->Disconnect();
	}
}

bool MCC::negotiationFinished() const
{
	return state() == ST_NEGOTIATION_FINISHED;
}

bool MCC::negotiationAgreement() const
{
	return state() == ST_NEGOTIATION_FINISHED;
}

bool MCC::registerIntoYellowPages()
{
	// Create message header and data
	PacketHeader packetHead;
	packetHead.packetType = PacketType::RegisterMCC;
	packetHead.srcAgentId = id();
	packetHead.dstAgentId = -1;
	PacketRegisterMCC packetData;
	packetData.itemId = _contributedItemId;

	// Serialize message
	OutputMemoryStream stream;
	packetHead.Write(stream);
	packetData.Write(stream);

	return sendPacketToYellowPages(stream);
}

void MCC::unregisterFromYellowPages()
{
	// Create message
	PacketHeader packetHead;
	packetHead.packetType = PacketType::UnregisterMCC;
	packetHead.srcAgentId = id();
	packetHead.dstAgentId = -1;
	PacketUnregisterMCC packetData;
	packetData.itemId = _contributedItemId;

	// Serialize message
	OutputMemoryStream stream;
	packetHead.Write(stream);
	packetData.Write(stream);

	sendPacketToYellowPages(stream);
}
