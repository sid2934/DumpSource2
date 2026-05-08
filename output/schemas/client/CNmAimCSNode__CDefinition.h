// MGetKV3ClassDefaults = {
//	"_class": "CNmAimCSNode::CDefinition",
//	"m_nNodeIdx": -1,
//	"m_nChildNodeIdx": -1,
//	"m_nVerticalAngleNodeIdx": -1,
//	"m_nHorizontalAngleNodeIdx": -1,
//	"m_nWeaponCategoryNodeIdx": -1,
//	"m_nWeaponTypeNodeIdx": -1,
//	"m_nWeaponActionNodeIdx": -1,
//	"m_nWeaponDropNodeIdx": -1,
//	"m_nIsDefusingNodeIdx": -1,
//	"m_nCrouchWeightNodeIdx": -1,
//	"m_flHandIKBlendInTimeSeconds": 0.000000,
//	"m_flActionBlendTimeSeconds": 0.000000,
//	"m_flPlantingBlendTimeSeconds": 0.000000
//}
class CNmAimCSNode::CDefinition : public CNmPassthroughNode::CDefinition
{
	int16 m_nVerticalAngleNodeIdx;
	int16 m_nHorizontalAngleNodeIdx;
	int16 m_nWeaponCategoryNodeIdx;
	int16 m_nWeaponTypeNodeIdx;
	int16 m_nWeaponActionNodeIdx;
	int16 m_nWeaponDropNodeIdx;
	int16 m_nIsDefusingNodeIdx;
	int16 m_nCrouchWeightNodeIdx;
	float32 m_flHandIKBlendInTimeSeconds;
	float32 m_flActionBlendTimeSeconds;
	float32 m_flPlantingBlendTimeSeconds;
};
