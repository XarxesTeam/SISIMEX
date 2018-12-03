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
	{
		_mccRegisterIndex = 0;
		queryMCCsForItem(_requestedItemId);
		setState(ST_REQUESTING_MCCs);
	}
	break;

	case ST_ITERATING_OVER_MCCs:
	{
		/// TODO: Handle this state
		if (_mccRegisters.size() > 0 && _mccRegisters.size() > _mccRegisterIndex)
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
			setState(ST_WAITING_MCCs_RESPONSE);
		}
		else
		{
			setState(ST_NEGOTIATION_FINISHED);
			removeChildUCP();
		}
		
	}
	break;
	
	case ST_NEGOTIATING:
	{
		if (_ucp->Finished())
		{
			negotiation_result = _ucp->negotiation_result == true;

			removeChildUCP();

			if (negotiation_result == true)
			{
				setState(ST_NEGOTIATION_FINISHED);
			}
			else
			{
				setState(ST_ITERATING_OVER_MCCs);
			}
		}
	}
	break;

	}
}

void MCP::stop()
{
	// TODO: Destroy the underlying search hierarchy (UCP->MCP->UCP->...)
	removeChildUCP(); //This call child ucp stop

	destroy();
}

void MCP::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	const PacketType packetType = packetHeader.packetType;

	switch (packetType)
	{
	case PacketType::ReturnMCCsForItem:
	{
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
			}

			// Store the returned MCCs from YP
			_mccRegisters.swap(packetData.mccAddresses);
			negotiation_result = false;

			// Select the first MCC to negociate
			_mccRegisterIndex = 0;
			setState(ST_ITERATING_OVER_MCCs);

			socket->Disconnect();
		}
		else
		{
			wLog << "OnPacketReceived() - PacketType::ReturnMCCsForItem was unexpected.";
		}
	}
	break;

	case PacketType::MCCNegotiationResponse:
	{
		if (state() == ST_WAITING_MCCs_RESPONSE)
		{
			PacketMCCNegotiationResponse mcc_negotiation_response;
			mcc_negotiation_response.Read(stream);

			if (mcc_negotiation_response.accepted)
			{
				iLog << "MCP Starting Negotiation";
				createChildUCP(mcc_negotiation_response.uccAgent);
				setState(ST_NEGOTIATING);
			}
			else
			{
				setState(ST_ITERATING_OVER_MCCs);
				_mccRegisterIndex += 1;
			}
		}
	}
	break;
	}
}

bool MCP::negotiationFinished() const
{
	return state() == ST_NEGOTIATION_FINISHED;
}

bool MCP::negotiationAgreement() const
{
	return negotiation_result;
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
	_ucp.reset();
	_ucp = App->agentContainer->createUCP(node(), _requestedItemId, _contributedItemId, uccAgent, _searchDepth);
	iLog << "UCP Created";
}

void MCP::removeChildUCP()
{
	if (_ucp.get())
	{
		_ucp->stop();
		_ucp.reset();
		iLog << "UCP Removed";
	}
}
