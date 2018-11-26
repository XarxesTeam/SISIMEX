#include "MCP.h"
#include "UCP.h"
#include "Application.h"
#include "ModuleAgentContainer.h"


enum State
{
	ST_INIT,
	ST_REQUESTING_MCCs,
	ST_ITERATING_OVER_MCCs,

	// TODO: Other states
	ST_WAITING_MCCs_RESPONSE,
	ST_NEGOTIATING,

	ST_NEGOTIATION_FINISHED
};

MCP::MCP(Node *node, uint16_t requestedItemID, uint16_t contributedItemID, unsigned int searchDepth) :
	Agent(node),
	_requestedItemId(requestedItemID),
	_contributedItemId(contributedItemID),
	_searchDepth(searchDepth),
	_mccRegisterIndex(0)
{
	setState(ST_INIT);
}

MCP::~MCP()
{
}

void MCP::update()
{
	switch (state())
	{
	case ST_INIT:
		_mccRegisterIndex = 0;
		queryMCCsForItem(_requestedItemId);
		setState(ST_REQUESTING_MCCs);
		break;

	case ST_ITERATING_OVER_MCCs:
	{
		/// TODO: Handle this state
		if (_mccRegisters.size() > 0)
		{
			AgentLocation curr = _mccRegisters[_mccRegisterIndex];
			OutputMemoryStream stream;

			PacketHeader packetHead;
			packetHead.srcAgentId = id();
			packetHead.dstAgentId = curr.agentId;
			packetHead.packetType = PacketType::NegotiationMCPPetition;
			packetHead.Write(stream);

			sendPacketToAgent(curr.hostIP, curr.hostPort, stream);
			iLog << "Negotiation Petition Sent to MCCs";
		}
		else
			wLog << "No MCCs Registered";
		setState(ST_WAITING_MCCs_RESPONSE);
	}
	break;

		/// TODO: Handle other states
	case ST_WAITING_MCCs_RESPONSE:
	{
		
	}
	break;

	default:;
	}
}

void MCP::stop()
{
	// TODO: Destroy the underlying search hierarchy (UCP->MCP->UCP->...)

	destroy();
}

void MCP::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	const PacketType packetType = packetHeader.packetType;

	switch (packetType)
	{
	case PacketType::ReturnMCCsForItem:
		if (state() == ST_REQUESTING_MCCs)
		{
			// Read the packet
			PacketReturnMCCsForItem packetData;
			packetData.Read(stream);

			// Log the returned MCCs
			for (auto &mccdata : packetData.mccAddresses)
			{
				uint16_t agentId = mccdata.agentId;
				const std::string &hostIp = mccdata.hostIP;
				uint16_t hostPort = mccdata.hostPort;
				//iLog << " - MCC: " << agentId << " - host: " << hostIp << ":" << hostPort;
			}

			// Store the returned MCCs from YP
			_mccRegisters.swap(packetData.mccAddresses);

			// Select the first MCC to negociate
			_mccRegisterIndex = 0;
			setState(ST_ITERATING_OVER_MCCs);

			socket->Disconnect();
		}
		else
		{
			wLog << "OnPacketReceived() - PacketType::ReturnMCCsForItem was unexpected.";
		}
		break;

		// TODO: Handle other packets
	case PacketType::MCCNegotiationResponse:
	{
		InputMemoryStream stream;
		PacketHeader packetHead;
		packetHead.Read(stream);

		PacketMCCNegotiationResponse packetData;
		packetData.Read(stream);
		if (packetData.accepted)
		{
			iLog << "MCP Starting Negotiation";
			createChildUCP(packetData.uccAgent);
			setState(ST_NEGOTIATING);
		}
		else
		{
			setState(ST_ITERATING_OVER_MCCs);
			_mccRegisterIndex++;
		}
	}
		break;
	
	default:
		wLog << "OnPacketReceived() - Unexpected PacketType.";
	}
}

bool MCP::negotiationFinished() const
{
	return state() == ST_NEGOTIATION_FINISHED;
}

bool MCP::negotiationAgreement() const
{
	return false; // TODO: Did the child UCP find a solution?
}


bool MCP::queryMCCsForItem(int itemId)
{
	// Create message header and data
	PacketHeader packetHead;
	packetHead.packetType = PacketType::QueryMCCsForItem;
	packetHead.srcAgentId = id();
	packetHead.dstAgentId = -1;
	PacketQueryMCCsForItem packetData;
	packetData.itemId = _requestedItemId;

	// Serialize message
	OutputMemoryStream stream;
	packetHead.Write(stream);
	packetData.Write(stream);

	// 1) Ask YP for MCC hosting the item 'itemId'
	return sendPacketToYellowPages(stream);
}

void MCP::createChildUCP(AgentLocation& uccAgent)
{
	iLog << "UCP Created";
	App->agentContainer->createUCP(node(), requestedItemId(), contributedItemId(), uccAgent, searchDepth());
}