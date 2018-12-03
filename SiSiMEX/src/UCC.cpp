#include "UCC.h"

enum UCC_State
{
	UCC_WAITING_ITEM_REQUEST,
	UCC_WAITING_CONSTRAIN,

	UCC_FINISHED
};

UCC::UCC(Node *node, uint16_t contributedItemId, uint16_t constraintItemId) : Agent(node), contributedItemId(contributedItemId), constraintItemId(constraintItemId)
{
	setState(UCC_WAITING_ITEM_REQUEST);
}

UCC::~UCC()
{

}

void UCC::stop()
{
	destroy();
}

void UCC::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	PacketType packetType = packetHeader.packetType;

	switch (packetType)
	{
	case PacketType::UCPItemRequest:
	{
		if (state() == UCC_WAITING_ITEM_REQUEST)
		{
			PacketUCPItemRequest ucp_item_request;
			ucp_item_request.Read(stream);

			if (ucp_item_request.itemId == contributedItemId)
			{
				PacketHeader header;
				header.packetType = PacketType::UCPConstrainRequest;
				header.dstAgentId = packetHeader.srcAgentId;
				header.srcAgentId = id();

				PacketUCPConstrainRequest ucp_constrain_request;
				ucp_constrain_request.itemId = constraintItemId;

				OutputMemoryStream stream;
				header.Write(stream);
				ucp_constrain_request.Write(stream);

				socket->SendPacket(stream.GetBufferPtr(), stream.GetSize());

				setState(UCC_WAITING_CONSTRAIN);
			}
		}
	}
	break;

	case PacketType::UCCConstrainRequestConclusion:
	{
		if (state() == UCC_WAITING_CONSTRAIN)
		{
			PacketUCPConstrainRequestConclusion ucp_constrain_request_conclusion;
			ucp_constrain_request_conclusion.Read(stream);
			negotiation_result = ucp_constrain_request_conclusion.negotiation_result;

			PacketHeader header;
			header.packetType = PacketType::UCCNegotiationConclusion;
			header.dstAgentId = packetHeader.srcAgentId;
			header.srcAgentId = id();

			OutputMemoryStream out_stream;
			header.Write(out_stream);
			ucp_constrain_request_conclusion.Write(out_stream);
			socket->SendPacket(out_stream.GetBufferPtr(), out_stream.GetSize());
		}
	}
	break;
	}
}

bool UCC::Finished() const
{
	return state() == UCC_State::UCC_FINISHED;
}
