#include "ModuleNodeCluster.h"
#include "ModuleNetworkManager.h"
#include "ModuleAgentContainer.h"
#include "Application.h"
#include "Log.h"
#include "Packets.h"
#include "imgui/imgui.h"
#include <sstream>

enum State {
	STOPPED,
	STARTING,
	RUNNING,
	STOPPING
};

bool ModuleNodeCluster::init()
{
	state = STOPPED;

	return true;
}

bool ModuleNodeCluster::start()
{
	state = STARTING;

	return true;
}

bool ModuleNodeCluster::update()
{
	bool ret = true;

	switch (state)
	{
	case STARTING:
		if (startSystem()) {
			state = RUNNING;
		} else {
			state = STOPPED;
			ret = false;
		}
		break;
	case RUNNING:
		runSystem();
		break;
	case STOPPING:
		stopSystem();
		state = STOPPED;
		break;
	}

	return ret;
}

bool ModuleNodeCluster::updateGUI()
{
	ImGui::Begin("Node cluster");

	if (state == RUNNING)
	{
		int itemsCount = 0;
		for (auto node : _nodes) {
			itemsCount += (int)node->itemList().numItems();
		}
		ImGui::TextWrapped("# items in the cluster: %d", itemsCount);

		int missingItemsCount = 0;
		for (auto node : _nodes) {
			missingItemsCount += (int)node->itemList().numMissingItems();
		}
		ImGui::TextWrapped("# missing items in the cluster: %d", missingItemsCount);

		ImGui::Separator();

		int nodeId = 0;
		for (auto &node : _nodes)
		{
			ImGui::PushID(nodeId);

			ImGuiTreeNodeFlags flags = 0;
			std::string nodeLabel = StringUtils::Sprintf("Node %d", nodeId);

			if (ImGui::CollapsingHeader(nodeLabel.c_str(), flags))
			{
				if (ImGui::TreeNodeEx("Items", flags))
				{
					auto &itemList = node->itemList();

					for (int itemId = 0; itemId < MAX_ITEMS; ++itemId)
					{
						unsigned int numItems = itemList.numItemsWithId(itemId);
						if (numItems == 1)
						{
							ImGui::Text("Item %d", itemId);
						}
						else if (numItems > 1)
						{
							ImGui::Text("Item %d (x%d)", itemId, numItems);
						}
					}

					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("MCCs", flags))
				{
					for (auto mcc : _mccs) {
						if (mcc->node()->id() == nodeId)
						{
							ImGui::Text("MCC %d", mcc->id());
							ImGui::Text(" - Contributed Item ID: %d", mcc->contributedItemId());
							ImGui::Text(" - Constraint Item ID: %d", mcc->constraintItemId());
						}
					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("MCPs", flags))
				{
					for (auto mcp : _mcps) {
						if (mcp->node()->id() == nodeId)
						{
							ImGui::Text("MCP %d", mcp->id());
							ImGui::Text(" - Requested Item ID: %d", mcp->requestedItemId());
							ImGui::Text(" - Contributed Item ID: %d", mcp->contributedItemId());
						}
					}
					ImGui::TreePop();
				}
			}

			ImGui::PopID();
			nodeId++;
		}
	}

	ImGui::End();

	if (state == RUNNING)
	{
		// NODES / ITEMS MATRIX /////////////////////////////////////////////////////////

		ImGui::Begin("Nodes/Items Matrix");

		static ItemId selectedItem = 0;
		static unsigned int selectedNode = 0;
		static int comboItem = NULL_ITEM_ID;

		ImGui::Text("Item ID ");
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.6f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 0.5f));
		for (ItemId itemId = 0U; itemId < MAX_ITEMS; ++itemId)
		{
			ImGui::SameLine();
			std::ostringstream oss;
			oss << itemId;
			ImGui::Button(oss.str().c_str(), ImVec2(20, 20));
			if (itemId < MAX_ITEMS - 1) ImGui::SameLine();
		}
		ImGui::PopStyleColor(3);

		ImGui::Separator();

		for (auto nodeIndex = 0U; nodeIndex < _nodes.size(); ++nodeIndex)
		{
			ImGui::Text("Node %02u ", nodeIndex);
			ImGui::SameLine();

			for (ItemId itemId = 0U; itemId < MAX_ITEMS; ++itemId)
			{
				unsigned int numItems = _nodes[nodeIndex]->itemList().numItemsWithId(itemId);

				if (numItems == 0)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.5f*numItems));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, 1.0f, 0.0f, 0.3f*numItems));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, 1.0f, 0.0f, 0.2f*numItems));
				}

				const int buttonId = nodeIndex * MAX_ITEMS + itemId;
				std::ostringstream oss;
				oss << numItems;
				oss << "##" << buttonId;

				if (ImGui::Button(oss.str().c_str(), ImVec2(20, 20))) {
					selectedNode = nodeIndex;
					selectedItem = itemId;
					comboItem = 0;
					ImGui::OpenPopup("ItemOps");
				}

				ImGui::PopStyleColor(3);

				if (itemId < MAX_ITEMS - 1) ImGui::SameLine();
			}
		}

		// Context menu to spawn agents
		if (ImGui::BeginPopup("ItemOps"))
		{
			int numberOfItems = _nodes[selectedNode]->itemList().numItemsWithId(selectedItem);

			// If it is a missing item...
			if (numberOfItems == 0)
			{
				int petitionedItem = selectedItem;

				// Check if we have spare items
				std::vector<std::string> comboStrings;
				std::vector<int> itemIds;
				for (ItemId itemId = 0; itemId < MAX_ITEMS; ++itemId) {
					if (_nodes[selectedNode]->itemList().numItemsWithId(itemId) > 1)
					{
						std::ostringstream oss;
						oss << itemId;
						comboStrings.push_back(oss.str());
						itemIds.push_back(itemId);
					}
				}

				std::vector<const char *> comboCStrings;
				for (auto &s : comboStrings) { comboCStrings.push_back(s.c_str()); }

				if (itemIds.size() > 0)
				{
					ImGui::Text("Create MultiCastPetitioner?");
					ImGui::Separator();
					ImGui::Text("Node %d", selectedNode);
					ImGui::Text(" - Petition: %d", petitionedItem);

					ImGui::Combo("Contribution", &comboItem, (const char **)&comboCStrings[0], comboCStrings.size());
					if (ImGui::Button("Spawn MCP")) {
						int contributedItem = itemIds[comboItem];
						spawnMCP(selectedNode, petitionedItem, contributedItem);
						ImGui::CloseCurrentPopup();
					}
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0));
					ImGui::Text("No spare items available to create an MCP.");
					ImGui::PopStyleColor(1);
				}
			}
			// Else if we have at least 2 items, we can contribute with the spare ones
			else if (numberOfItems > 1)
			{
				int contributedItem = selectedItem;

				// Check if we have missing items
				std::vector<std::string> comboStrings;
				std::vector<int> itemIds;
				for (ItemId itemId = 0; itemId < MAX_ITEMS; ++itemId) {
					if (_nodes[selectedNode]->itemList().numItemsWithId(itemId) == 0)
					{
						std::ostringstream oss;
						oss << itemId;
						comboStrings.push_back(oss.str());
						itemIds.push_back(itemId);
					}
				}

				std::vector<const char *> comboCStrings;
				for (auto &s : comboStrings) { comboCStrings.push_back(s.c_str()); }

				if (itemIds.size() > 0)
				{
					ImGui::Text("Create MultiCastContributor?");
					ImGui::Separator();
					ImGui::Text("Node %d", selectedNode);
					ImGui::Text(" - Contribution: %d", contributedItem);

					ImGui::Combo("Constraint", &comboItem, (const char **)&comboCStrings[0], comboCStrings.size());
					if (ImGui::Button("Spawn MCP")) {
						int constraintItem = itemIds[comboItem];
						spawnMCC(selectedNode, contributedItem, constraintItem);
						ImGui::CloseCurrentPopup();
					}
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0));
					ImGui::Text("No missing items to create an MCC.");
					ImGui::PopStyleColor(1);
				}
			}
			else if (numberOfItems == 1)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0));
				ImGui::Text("You already have this item: cannot create MCP.");
				ImGui::Text("You don't have more than 1 item: Cannot create MCC.");
				ImGui::PopStyleColor(1);
			}
			ImGui::EndPopup();
		}

		ImGui::End();
	}

	return true;
}

bool ModuleNodeCluster::stop()
{
	state = STOPPING;

	return true;
}

bool ModuleNodeCluster::cleanUp()
{
	return true;
}

void ModuleNodeCluster::OnAccepted(TCPSocketPtr socket)
{
	// Nothing to do
}

void ModuleNodeCluster::OnPacketReceived(TCPSocketPtr socket, InputMemoryStream & stream)
{
	//iLog << "OnPacketReceived";

	PacketHeader packetHead;
	packetHead.Read(stream);

	// Get the agent
	auto agentPtr = App->agentContainer->getAgent(packetHead.dstAgentId);
	if (agentPtr != nullptr)
	{
		agentPtr->OnPacketReceived(socket, packetHead, stream);
	}
	else
	{
		eLog << "Couldn't find agent: " << packetHead.dstAgentId;
	}
}

void ModuleNodeCluster::OnDisconnected(TCPSocketPtr socket)
{
	// Nothing to do
}

bool ModuleNodeCluster::startSystem()
{
	iLog << "---------------------------------------------------";
	iLog << "               SiSiMEX Multi-Agents                ";
	iLog << "---------------------------------------------------";
	iLog << "";

	// Create listen socket
	TCPSocketPtr listenSocket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
	if (listenSocket == nullptr) {
		eLog << "SocketUtil::CreateTCPSocket() failed";
		return false;
	}
	iLog << " - Server Listen socket created";

	// Bind
	const int port = LISTEN_PORT_AGENTS;
	SocketAddress bindAddress(port); // localhost:LISTEN_PORT_AGENTS
	listenSocket->SetReuseAddress(true);
	int res = listenSocket->Bind(bindAddress);
	if (res != NO_ERROR) { return false; }
	iLog << " - Socket Bind to interface 127.0.0.1:" << LISTEN_PORT_AGENTS;

	// Listen mode
	res = listenSocket->Listen();
	if (res != NO_ERROR) { return false; }
	iLog << " - Socket entered in Listen state...";

	// Add the socket to the manager
	App->networkManager->SetDelegate(this);
	App->networkManager->AddSocket(listenSocket);

//#define RANDOM
#ifdef RANDOM
#define MAX_NODES 40
	// Initialize nodes
	for (int i = 0; i < MAX_NODES; ++i)
	{
		// Create and intialize nodes
		NodePtr node = std::make_shared<Node>(i);
		node->itemList().initializeComplete();
		_nodes.push_back(node);
	}

	// Randomize
	for (int j = 0; j < MAX_ITEMS; ++j)
	{
		for (int i = 0; i < MAX_NODES; ++i)
		{
			ItemId itemId = rand() % MAX_ITEMS;
			while (_nodes[i]->itemList().numItemsWithId(itemId) == 0) {
				itemId = rand() % MAX_ITEMS;
			}
			_nodes[i]->itemList().removeItem(itemId);
			_nodes[(i + 1) % MAX_NODES]->itemList().addItem(itemId);
		}
	}

	//// Create MCP agents for each node
	//for (int i = 0; i < MAX_NODES; ++i)
	//{
	//	// Spawn MCC (MultiCastContributors) one for each spare item
	//	NodePtr node = _nodes[i];
	//	ItemList spareItems = node->itemList().getSpareItems();
	//	for (auto item : spareItems.items()) {
	//		spawnMCC(i, item.id());
	//	}
	//}
#else
	_nodes.push_back(std::make_shared<Node>((int)_nodes.size()));
	_nodes.push_back(std::make_shared<Node>((int)_nodes.size()));
	_nodes.push_back(std::make_shared<Node>((int)_nodes.size()));
	_nodes.push_back(std::make_shared<Node>((int)_nodes.size()));
	_nodes[0]->itemList().addItem(ItemId(0));
	_nodes[0]->itemList().addItem(ItemId(0));
	_nodes[1]->itemList().addItem(ItemId(1));
	_nodes[1]->itemList().addItem(ItemId(1));
	_nodes[2]->itemList().addItem(ItemId(2));
	_nodes[2]->itemList().addItem(ItemId(2));
	_nodes[3]->itemList().addItem(ItemId(3));
	_nodes[3]->itemList().addItem(ItemId(3));

	// Defines to clarify the next lines
#	define NODE(x) x
#	define CONTRIBUTION(x) x
#	define CONSTRAINT(x) x

	spawnMCC(NODE(1), CONTRIBUTION(1), CONSTRAINT(2)); // Node 1 offers 1 but wants 2
	spawnMCC(NODE(2), CONTRIBUTION(2), CONSTRAINT(3)); // Node 2 offers 2 but wants 3
	spawnMCC(NODE(3), CONTRIBUTION(3), CONSTRAINT(0)); // Node 3 offers 3 but wants 0
#endif

	return true;
}

void ModuleNodeCluster::runSystem()
{
	// Check the results of agents
	std::vector<MCC*> mccsAlive;
	std::vector<MCP*> mcpsAlive;
	for (auto mcc : _mccs) {
		if (!mcc->isValid()) { continue; }
		if (mcc->negotiationFinished()) {
			Node *node = mcc->node();
			node->itemList().removeItem(mcc->contributedItemId());
			if (mcc->constraintItemId() != NULL_ITEM_ID) {
				node->itemList().addItem(mcc->constraintItemId());
			}
			mcc->stop();
		}
		else {
			mccsAlive.push_back(mcc);
		}
	}
	for (auto mcp : _mcps) {
		if (!mcp->isValid()) { continue; }
		if (mcp->negotiationFinished()) {
			if (mcp->negotiationAgreement()) {
				Node *node = mcp->node();
				node->itemList().addItem(mcp->requestedItemId());
			}
			mcp->stop();
		}
		else {
			mcpsAlive.push_back(mcp);
		}
	}
	_mcps.swap(mcpsAlive);
	_mccs.swap(mccsAlive);
}

void ModuleNodeCluster::stopSystem()
{
}

void ModuleNodeCluster::spawnMCP(int nodeId, int requestedItemId, int contributedItemId)
{
	iLog << "Spawn MCP for node " << nodeId << " petitioning item " << requestedItemId << " in exchange of item " << contributedItemId;
	if (nodeId >= 0 && nodeId < (int)_nodes.size()) {

		// Done for you ;-)
		// Here we simply create the MCP agent using the agent container module, specifying:
		// 1) The item it requests
		// 2) The item it provides
		// and finally insets it into a list to track its lifetime in the ModuleNodeCluster::runSystem() method

		NodePtr node = _nodes[nodeId];
		MCPPtr mcp = App->agentContainer->createMCP(node.get(), requestedItemId, contributedItemId);
		_mcps.push_back(mcp.get());
	}
	else {
		wLog << "Could not find node with ID " << nodeId;
	}
}

void ModuleNodeCluster::spawnMCC(int nodeId, int contributedItemId, int constraintItemId)
{
	iLog << "Spawn MCC for node " << nodeId << " contributing item " << contributedItemId << " - constraint item " << constraintItemId;
	if (nodeId >= 0 && nodeId < (int)_nodes.size()) {
		NodePtr node = _nodes[nodeId];
		MCCPtr mcc = App->agentContainer->createMCC(node.get(), contributedItemId, constraintItemId);
		_mccs.push_back(mcc.get());
	}
	else {
		wLog << "Could not find node with ID " << nodeId;
	}
}
