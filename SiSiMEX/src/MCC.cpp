#include "MCC.h"
#include "UCC.h"
#include "Application.h"
#include "ModuleAgentContainer.h"
#include "ModuleNodeCluster.h"

enum State
{
	ST_INIT,
	ST_REGISTERING,
	ST_IDLE,
	
	ST_NEGOTIATING,
	ST_NEGOTIATION_END,

	ST_FINISHED
};

MCC::MCC(Node * node, uint16_t contributedItemId, uint16_t constraintItemId, uint16_t _itemsNum) : Agent(node), _contributedItemId(contributedItemId), _constraintItemId(constraintItemId), _itemsNum(_itemsNum)
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

	case ST_NEGOTIATING:
		if (_ucc->Finished() == true)
		{
			bool ucc_result = _ucc->negotiation_result;
			uint16_t ucc_contributedItemsNum = _ucc->contributedItemsNum;
			uint16_t ucc_constraintItemsNum = _ucc->constraintItemsNum;

			removeChildUCC();

			if (ucc_result)
			{
				setState(ST_NEGOTIATION_END);
				_contributedItemsNum = ucc_contributedItemsNum;
				_constrainItemsNum = ucc_constraintItemsNum;
			}
			else
			{
				setState(ST_IDLE);
				App->modNodeCluster->RemoveNegotiation(node()->id(), _constrainItemsNum);
			}
		}
		break;

	case ST_FINISHED:
		{
		destroy();
		}
		break;

	}
}

void MCC::stop()
{
	// Destroy hierarchy below this agent (only a UCC, actually)
	removeChildUCC(); //Call child ucc stop

	unregisterFromYellowPages();
	setState(ST_FINISHED);
}


void MCC::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &_stream)
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

	case PacketType::NegotiationMCPPetition:
	{
		iLog << "MCC: NegotiationMCPPetition received";
		OutputMemoryStream stream;
		PacketHeader packetHead;
		packetHead.dstAgentId = packetHeader.srcAgentId;
		packetHead.srcAgentId = id();
		packetHead.packetType = PacketType::MCCNegotiationResponse;

		PacketMCCNegotiationResponse mccResponsePacket;

		PacketMCPNegotiationPetitionItemsNum mcpNegItemsNum;
		mcpNegItemsNum.Read(_stream);

		if (state() == ST_IDLE && CheckInteractionAndItemsNum(mcpNegItemsNum.itemsNum))
		{
			setState(ST_NEGOTIATING);
			iLog << "Starting negotiating";

			App->modNodeCluster->AddNegotation(node()->id(), _contributedItemId);
			createChildUCC();
			
			AgentLocation uccAgent;
			uccAgent.hostIP = socket->RemoteAddress().GetIPString();
			uccAgent.agentId = _ucc->id();
			uccAgent.hostPort = LISTEN_PORT_AGENTS;
			
			mccResponsePacket.accepted = true;
			mccResponsePacket.uccAgent = uccAgent;
		}
		else
		{
			mccResponsePacket.accepted = false;
			iLog << "MCC can't negotiate now";
		}

		packetHead.Write(stream);
		mccResponsePacket.Write(stream);
		socket->SendPacket(stream.GetBufferPtr(), stream.GetSize());

	}
	break;
	case PacketType::UnregisterMCC:
	{
		setState(ST_FINISHED);
		socket->Disconnect();
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
	// If this agent finished, means that it was an agreement
	// Otherwise, it would return to state ST_IDLE
	return state() == ST_NEGOTIATION_END;
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

bool MCC::CheckInteractionAndItemsNum(uint16_t num_request)
{
	//Check if the agent is already in the list so we can't interact with 
	if (App->modNodeCluster->CheckAgentInteraction(node()->id(), _contributedItemId))
	{
		return num_request < _itemsNum;
	}
	return false;
}

void MCC::createChildUCC()
{
	_ucc.reset();
	_ucc = App->agentContainer->createUCC(node(), _contributedItemId, _constraintItemId, 1);
	iLog << "UCC Created";
}

void MCC::removeChildUCC()
{
	if (_ucc.get() != nullptr)
	{
		_ucc->stop();
		_ucc.reset();
		iLog << "UCC Removed";
	}
}
