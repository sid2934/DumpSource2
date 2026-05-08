// MGetKV3ClassDefaults = {
//	"_class": "CNmSnapWeaponNode::CDefinition",
//	"m_nNodeIdx": -1,
//	"m_nChildNodeIdx": -1,
//	"m_nFlashedAmountNodeIdx": -1,
//	"m_nWeaponCategoryNodeIdx": -1,
//	"m_nWeaponTypeNodeIdx": -1
//}
class CNmSnapWeaponNode::CDefinition : public CNmPassthroughNode::CDefinition
{
	int16 m_nFlashedAmountNodeIdx;
	int16 m_nWeaponCategoryNodeIdx;
	int16 m_nWeaponTypeNodeIdx;
};
