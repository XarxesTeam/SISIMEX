#include "ItemList.h"
#include "Globals.h"
#include <cassert>
#include <algorithm>
#include <string>

ItemList::ItemList()
{
}


ItemList::~ItemList()
{
}

void ItemList::initializeComplete()
{
	for (ItemId itemId = 0; itemId < MAX_ITEMS; ++itemId)
	{
		items[itemId] = 1;
	}
	numberOfItems = MAX_ITEMS;
	numberOfMissingItems = 0;
}

void ItemList::addItem(ItemId itemId, int num)
{
	assert(itemId < MAX_ITEMS && "ItemsList::addItem() - itemId out of bounds.");
	items[itemId] += num;
	numberOfItems += num;
	recomputeMissingItems();
}

void ItemList::removeItem(ItemId itemId, int num)
{
	assert(itemId < MAX_ITEMS && "ItemsList::removeItem() - itemId out of bounds.");
	assert(items[itemId] > 0 && "ItemsList::removeItem() - the list does not contain this item.");
	items[itemId] -= num;
	numberOfItems -= num;
	recomputeMissingItems();
}

unsigned int ItemList::numItemsWithId(ItemId itemId)
{
	assert(itemId < MAX_ITEMS && "ItemsList::numItemsWithId() - itemId out of bounds.");
	return items[itemId];
}

unsigned int ItemList::numItems() const
{
	return numberOfItems;
}

unsigned int ItemList::numMissingItems() const
{
	return numberOfMissingItems;
}

std::string ItemList::getItemName(unsigned int itemId) const
{
	std::string itemName = "No Name";
	switch (itemId)
	{
	case 0:
		itemName = "Stone";
		break;
	case 1:
		itemName = "Granite";
		break;
	case 2:
		itemName = "Polished Granite";
		break;
	case 3:
		itemName = "Polished Diorite";
		break;
	case 4:
		itemName = "Andesite";
		break;
	case 5:
		itemName = "Polished Andesite";
		break;
	case 6:
		itemName = "Grass";
		break;
	case 7:
		itemName = "Dirt";
		break;
	case 8:
		itemName = "Coarse Dirt";
		break;
	case 9:
		itemName = "Podzol";
		break;
	case 10:
		itemName = "Cobblestone";
		break;
	case 11:
		itemName = "OakWood Plank";
		break;
	case 12:
		itemName = "Spruce Wood Plank";
		break;
	case 13:
		itemName = "Birch Wood Plank";
		break;
	case 14:
		itemName = "Jungle Wood Plank";
		break;
	case 15:
		itemName = "Acacia Wood Plank";
		break;
	case 16:
		itemName = "Dark Oak Wood Plank";
		break;
	case 17:
		itemName = "Oak Sapling";
		break;
	case 18:
		itemName = "Spruce Sapling";
		break;
	case 19:
		itemName = "Birch Sapling";
		break;
	case 20:
		itemName = "Jungle Sapling";
		break;
	case 21:
		itemName = "Acacia Sapling";
		break;
	case 22:
		itemName = "Dark Oak Sapling";
		break;
	case 23:
		itemName = "Bedrock";
		break;
	case 24:
		itemName = "FlowingWater";
		break;
	case 25:
		itemName = "StillWater";
		break;
	case 26:
		itemName = "FlowingLava";
		break;
	case 27:
		itemName = "StillLava";
		break;
	case 28:
		itemName = "Sand";
		break;
	case 29:
		itemName = "RedSand";
		break;
	default:
		break;
	}
	return itemName.c_str();
}


void ItemList::recomputeMissingItems()
{
	numberOfMissingItems = 0;
	for (ItemId itemId = 0; itemId < MAX_ITEMS; ++itemId) {
		if (items[itemId] == 0) {
			numberOfMissingItems++;
		}
	}
}
