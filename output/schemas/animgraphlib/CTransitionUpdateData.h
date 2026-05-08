// MGetKV3ClassDefaults = {
//	"m_srcStateIndex": 204,
//	"m_destStateIndex": 204,
//	"m_nHandshakeMaskToDisableFirst": 76,
//	"m_bDisabled": 1
//}
class CTransitionUpdateData
{
	uint8 m_srcStateIndex;
	uint8 m_destStateIndex;
	bitfield:7 m_nHandshakeMaskToDisableFirst;
	bitfield:1 m_bDisabled;
};
