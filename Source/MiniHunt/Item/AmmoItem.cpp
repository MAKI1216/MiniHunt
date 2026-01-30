#include "MiniHunt/Item/AmmoItem.h"

AAmmoItem::AAmmoItem()
{
	//初始化默认值，无所谓，后面由蓝图设置
	ItemType = EItemType::EIT_Ammo;
	AmmoCount = 30;
	ItemName = TEXT("Rifle Ammo");
}