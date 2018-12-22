#include "UCC.h"

enum UCC_State
{
	UCC_WAITING_ITEM_REQUEST,
	UCC_WAITING_CONSTRAIN,

	UCC_FINISHED
};

UCC::UCC(Node * node, uint16_t contributedItemId, uint16_t constraintItemId, uint16_t contributedItemsNum) : Agent(node), contributedItemId(contributedItemId), constraintItemId(constraintItemId), contributedItemsNum(contributedItemsNum)
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

void UCC::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &_stream)
{
	PacketType packetType = packetHeader.packetType;

	switch (packetType)
	{
	case PacketType::UCPItemRequest:
	{
		if (state() == UCC_WAITING_ITEM_REQUEST)
		{
			setState(UCC_WAITING_CONSTRAIN);

			PacketHeader header;
			header.packetType = PacketType::UCPConstrainRequest;
			header.srcAgentId = id();
			header.dstAgentId = packetHeader.srcAgentId;

			PacketUCPConstrainRequest contrain_request;
			contrain_request.itemId = constraintItemId;

			OutputMemoryStream stream;
			header.Write(stream);
			contrain_request.Write(stream);

			socket->SendPacket(stream.GetBufferPtr(), stream.GetSize());
		}
	}
	break;
	case PacketType::UCCConstrainRequestConclusion:
	{
		if (state() == UCC_WAITING_CONSTRAIN)
		{
			setState(UCC_FINISHED);

			PacketUCPConstrainRequestConclusion constrain_results;
			constrain_results.Read(_stream);
			negotiation_result = constrain_results.negotiation_result;
			contributedItemsNum = constrain_results.constrain_num;
			constraintItemsNum = constrain_results.contributed_num;

			PacketHeader header;
			header.packetType = PacketType::UCCNegotiationConclusion;
			header.srcAgentId = id();
			header.dstAgentId = packetHeader.srcAgentId;

			PacketNegotiationConclusion negotiationConclusion;
			negotiationConclusion.negotiation_result = negotiation_result;
			
			OutputMemoryStream stream;
			header.Write(stream);
			negotiationConclusion.Write(stream);
			
			socket->SendPacket(stream.GetBufferPtr(), stream.GetSize());
		}
	}
	break;
	}
}

bool UCC::Finished() const
{
	return state() == UCC_State::UCC_FINISHED;
}
