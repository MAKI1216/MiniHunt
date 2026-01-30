#include "MiniHunt/Item/ScoreItem.h"

AScoreItem::AScoreItem()
{
	ItemType = EItemType::EIT_Score; // 记得去 ItemBase.h 把 EIT_Score 枚举加上
	ScoreValue = 10; // 默认值
	ItemName = TEXT("Score Data");
}