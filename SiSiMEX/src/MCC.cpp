#include "MCC.h"
#include "UCC.h"
#include "Application.h"
#include "ModuleAgentContainer.h"


enum State
{
	ST_INIT,
	ST_REGISTERING,
	ST_IDLE,
	
	// TODO: Other states
	ST_NEGOTIATING,

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
		}
		else {
			setState(ST_FINISHED);
		}
		break;

	case ST_REGISTERING:
		// See OnPacketReceived()
		break;

		// TODO: Handle other states
	case ST_NEGOTIATING:
		if (_ucc->Finished() == true)
		{
			negotiation_result = _ucc->negotiation_result;
			
			removeChildUCC();

			if (negotiation_result == false)
			{
				setState(ST_FINISHED);
			}
			else
			{
				setState(ST_IDLE);
			}
		}
		break;

	case ST_FINISHED:
		destroy();
	}
}

void MCC::stop()
{
	// Destroy hierarchy below this agent (only a UCC, actually)
	removeChildUCC(); //Call child ucc stop

	unregisterFromYellowPages();
	setState(ST_FINISHED);
}


void MCC::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	const PacketType packetType = packetHeader.packetType;

	switch (packetType)
	{
	case PacketType::RegisterMCCAck:
		if (state() == ST_REGISTERING)
		{
			setState(ST_IDLE);
			socket->Disconnect();
		}
		else
		{
			wLog << "OnPacketReceived() - PacketType::RegisterMCCAck was unexpected.";
		}
		break;

	/// TODO: Handle other packets

	case PacketType::NegotiationMCPPetition:
	{			
		iLog << "MCC: NegotiationMCPPetition received";
		OutputMemoryStream stream;
		PacketHeader packetHead;
		packetHead.dstAgentId = packetHeader.srcAgentId;
		packetHead.srcAgentId = packetHeader.dstAgentId;
		packetHead.packetType = PacketType::MCCNegotiationResponse;
		packetHead.Write(stream);

		PacketMCCNegotiationResponse mccResponsePacket;

		if (state() == ST_IDLE)
		{
			AgentLocation uccAgent;
			mccResponsePacket.accepted = true;
			createChildUCC();
			uccAgent.agentId = _ucc->id();
			uccAgent.hostIP = socket->RemoteAddress().GetIPString();
			uccAgent.hostPort = LISTEN_PORT_AGENTS;
			
			mccResponsePacket.uccAgent = uccAgent;
			
			setState(ST_NEGOTIATING);
			iLog << "Starting negotiating";
		}
		else
		{
			mccResponsePacket.accepted = false;
			iLog << "MCC can't negotiate now";
		}

		mccResponsePacket.Write(stream);
		socket->SendPacket(stream.GetBufferPtr(), stream.GetSize());
	}
	break;
	default:
		wLog << "OnPacketReceived() - Unexpected PacketType.";
	}
}

bool MCC::isIdling() const
{
	return state() == ST_IDLE;
}

bool MCC::negotiationFinished() const
{
	return state() == ST_FINISHED;
}

bool MCC::negotiationAgreement() const
{
	// If this agent finished, means that it was an agreement
	// Otherwise, it would return to state ST_IDLE
	return negotiation_result;
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

void MCC::createChildUCC()
{
	/// TODO: Create a unicast contributor
	_ucc.reset();
	_ucc = App->agentContainer->createUCC(node(), contributedItemId(), constraintItemId());
	iLog << "UCC Created";
}

void MCC::removeChildUCC()
{
	// TODO: Destroy the unicast contributor child
	if (_ucc.get() != nullptr)
	{
		_ucc->stop();
		_ucc.reset();
		iLog << "UCC Removed";
	}
}
