#include "UCP.h"
#include "MCP.h"
#include "Application.h"
#include "ModuleAgentContainer.h"


// TODO: Make an enum with the states
enum UCP_State
{
	UCP_INIT,
	UCP_WAITING_ITEM_REQUEST,

	UCP_CHECK_CONSTRAIN,
	UCP_SEND_CONSTRAIN,

	UCP_FINISHED
};

UCP::UCP(Node *node, uint16_t requestedItemId, uint16_t _contributedItemId, const AgentLocation &uccLocation, unsigned int depth) :	Agent(node), requestedItemId(requestedItemId), contributedItemId(_contributedItemId), uccLocation(uccLocation), depth(depth)
{
	/// TODO: Save input parameters
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
		PacketHeader header;
		header.packetType = PacketType::UCPItemRequest;
		header.dstAgentId = uccLocation.agentId;
		header.srcAgentId = id();

		PacketUCPItemRequest ucp_item_request;
		ucp_item_request.itemId = requestedItemId;

		OutputMemoryStream stream;
		header.Write(stream);
		ucp_item_request.Write(stream);
		
		sendPacketToAgent(uccLocation.hostIP, uccLocation.hostPort, stream);

		setState(UCP_WAITING_ITEM_REQUEST);
	}
	break;

	case UCP_State::UCP_CHECK_CONSTRAIN:
	{
		if (_mcp != nullptr && _mcp->negotiationFinished())
		{
			PacketHeader header;
			header.packetType = PacketType::UCCConstrainRequestConclusion;
			header.dstAgentId = uccLocation.agentId;
			header.srcAgentId = id();

			PacketUCPConstrainRequestConclusion ucp_constrain_request_conclusion;
			ucp_constrain_request_conclusion.negotiation_result = _mcp->negotiationAgreement();

			OutputMemoryStream stream;
			header.Write(stream);
			ucp_constrain_request_conclusion.Write(stream);

			sendPacketToAgent(uccLocation.hostIP, uccLocation.hostPort, stream);
		}
	}

	case UCP_State::UCP_FINISHED:
	{
		removeChildMCP();
	}
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
			PacketUCPItemRequest ucp_item_request;
			ucp_item_request.Read(stream);

			PacketHeader header;
			header.packetType = PacketType::UCCConstrainRequestConclusion;
			header.dstAgentId = packetHeader.srcAgentId;
			header.srcAgentId = id();

			PacketUCPConstrainRequestConclusion ucp_constrain_request_conclusion;

			if (depth > 10)
			{
				iLog << "Search depth limit reached!";
				ucp_constrain_request_conclusion.negotiation_result = false;
				setState(UCP_SEND_CONSTRAIN);
			}
			else if (ucp_item_request.itemId == contributedItemId)
			{
				iLog << "Search found same contributon item";
				ucp_constrain_request_conclusion.negotiation_result = true;
				setState(UCP_SEND_CONSTRAIN);
			}
			else
			{
				iLog << "Search contribution item don't match";
				createChildMCP(ucp_item_request.itemId);
				setState(UCP_CHECK_CONSTRAIN);
			}

			OutputMemoryStream out_stream;
			header.Write(out_stream);
			ucp_constrain_request_conclusion.Write(out_stream);

			socket->SendPacket(out_stream.GetBufferPtr(), out_stream.GetSize());
		}
	}
	break;

	case PacketType::UCCNegotiationConclusion:
	{
		if(state() == UCP_SEND_CONSTRAIN)
		{
			PacketNegotiationConclusion neg_conclusion;
			neg_conclusion.Read(stream);

			negotiation_result = neg_conclusion.negotiation_result;

			setState(UCP_FINISHED);
		}
	}
	break;
	}
}

void UCP::createChildMCP(uint16_t request_item_id)
{
	_mcp.reset();
	_mcp = App->agentContainer->createMCP(node(), request_item_id, contributedItemId, depth + 1);
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

bool UCP::Finished() const
{
	return state() == UCP_FINISHED;
}
