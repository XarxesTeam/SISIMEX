#include "UCP.h"
#include "MCP.h"
#include "Application.h"
#include "ModuleAgentContainer.h"

#define MAX_DEPTH 6

// TODO: Make an enum with the states
enum UCP_State
{
	UCP_INIT,
	UCP_WAITING_ITEM_REQUEST,

	UCP_CHECK_CONSTRAIN,
	UCP_SEND_CONSTRAIN,

	UCP_FINISHED
};

UCP::UCP(Node * node, uint16_t _currentItemsNum, uint16_t requestedItemId, uint16_t requestedItemsNum, uint16_t contributedItemId, uint16_t contributedItemsNum, const AgentLocation & uccAgent, unsigned int searchDepth) :	Agent(node), _currentItemsNum(_currentItemsNum), _requestedItemId(requestedItemId), _requestedItemsNum(requestedItemsNum), _contributedItemId(contributedItemId), _contributedItemsNum(contributedItemsNum), uccAgent(uccAgent), depth(searchDepth)
{
	setState(UCP_INIT);
}

UCP::~UCP()
{

}

void UCP::update()
{
	switch (state())
	{
	case UCP_State::UCP_INIT:
	{
		setState(UCP_WAITING_ITEM_REQUEST);

		PacketHeader header;
		header.packetType = PacketType::UCPItemRequest;
		header.dstAgentId = uccAgent.agentId;
		header.srcAgentId = id();

		OutputMemoryStream stream;
		header.Write(stream);

		sendPacketToAgent(uccAgent.hostIP, uccAgent.hostPort, stream);
	}
	break;

	case UCP_State::UCP_CHECK_CONSTRAIN:
	{
		if (_mcp->negotiationFinished())
		{
			setState(UCP_SEND_CONSTRAIN);
			
			PacketHeader header;
			header.packetType = PacketType::UCCConstrainRequestConclusion;
			header.srcAgentId = id();
			header.dstAgentId = uccAgent.agentId;
			
			PacketUCPConstrainRequestConclusion constrainRequestConclusion;
			constrainRequestConclusion.constrain_num = _requestedItemsNum;
			constrainRequestConclusion.negotiation_result = _mcp->negotiationAgreement();
			constrainRequestConclusion.contributed_num = _contributedItemsNum;

			OutputMemoryStream stream;
			header.Write(stream);
			constrainRequestConclusion.Write(stream);
			
			sendPacketToAgent(uccAgent.hostIP, uccAgent.hostPort, stream);
		}
	}
	break;

	case UCP_State::UCP_FINISHED:
		removeChildMCP();
		break;
	}
}

void UCP::stop()
{
	// TODO: Destroy search hierarchy below this agent
	removeChildMCP(); //This call child mcp stop
	destroy();
}

void UCP::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	PacketType packetType = packetHeader.packetType;

	switch (packetType)
	{
	case PacketType::UCPConstrainRequest:
	{
		if (state() == UCP_WAITING_ITEM_REQUEST)
		{
			PacketHeader header;
			header.packetType = PacketType::UCCConstrainRequestConclusion;
			header.srcAgentId = id();
			header.dstAgentId = packetHeader.srcAgentId;

			PacketUCPConstrainRequest item_request;
			PacketUCPConstrainRequestConclusion constrain_result;

			item_request.Read(stream);

			OutputMemoryStream stream;


			if (item_request.itemId == _contributedItemId)
			{
				setState(UCP_SEND_CONSTRAIN);

				constrain_result.negotiation_result = true;
				constrain_result.constrain_num = _requestedItemsNum;
				constrain_result.contributed_num = _contributedItemsNum;

				header.Write(stream);
				constrain_result.Write(stream);
			}
			else if (depth < MAX_DEPTH)
			{
				setState(UCP_CHECK_CONSTRAIN);

				int petitionItemsNum = 0; //Need get petitin items num
				int contributionItemsNum = 0; //Need get contrib items num

				if (_currentItemsNum < contributionItemsNum)
				{
					setState(UCP_SEND_CONSTRAIN);
					constrain_result.negotiation_result = false;

					header.Write(stream);
					constrain_result.Write(stream);
				}
				else
				{
					createChildMCP(item_request.itemId, petitionItemsNum, _contributedItemsNum);
				}
			}
			else
			{
				setState(UCP_SEND_CONSTRAIN);

				constrain_result.negotiation_result = false;

				header.Write(stream);
				constrain_result.Write(stream);
			}

			socket->SendPacket(stream.GetBufferPtr(), stream.GetSize());
		}
	}
	break;

	case PacketType::UCCNegotiationConclusion:
	{
		if (state() == UCP_SEND_CONSTRAIN)
		{
			PacketNegotiationConclusion item_ack;
			item_ack.Read(stream);
			negotiation_result = item_ack.negotiation_result;
			setState(UCP_FINISHED);
		}
	}
	break;
	}
}

bool UCP::Finished() const
{
	return state() == UCP_FINISHED;
}

void UCP::createChildMCP(uint16_t request, uint16_t requestedItemsNum, uint16_t contributedItemsNum)
{
	removeChildMCP();
	_mcp = App->agentContainer->createMCP(node(), _currentItemsNum, request, requestedItemsNum, _contributedItemId, contributedItemsNum, depth + 1);
	iLog << "MCP Created";
}

void UCP::removeChildMCP()
{
	if (_mcp.get())
	{
		_mcp->stop();
		_mcp.reset();
		iLog << "MCP Removed";
	}
}

