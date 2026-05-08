// MGetKV3ClassDefaults = {
//	"_class": "CTakeDamageInfo",
//	"m_vecDamageForce":
//	[
//		0.000000,
//		0.000000,
//		0.000000
//	],
//	"m_vecDamagePosition": null,
//	"m_vecReportedPosition": null,
//	"m_vecDamageDirection":
//	[
//		0.000000,
//		0.000000,
//		0.000000
//	],
//	"m_hInflictor": null,
//	"m_hAttacker": null,
//	"m_hAbility": null,
//	"m_flDamage": 0.000000,
//	"m_flTotalledDamage": 0.000000,
//	"m_bitsDamageType": "",
//	"m_iDamageCustom": 0,
//	"m_iAmmoType": "",
//	"m_flOriginalDamage": 0.000000,
//	"m_bShouldBleed": false,
//	"m_bShouldSpark": false,
//	"m_nDamageFlags": "",
//	"m_iHitGroupId": "HITGROUP_INVALID",
//	"m_nNumObjectsPenetrated": 0,
//	"m_flFriendlyFireDamageReductionRatio": 1.000000,
//	"m_bStoppedBullet": false,
//	"m_DestructibleHitGroupRequests":
//	[
//	]
//}
class CTakeDamageInfo
{
	Vector m_vecDamageForce;
	VectorWS m_vecDamagePosition;
	VectorWS m_vecReportedPosition;
	Vector m_vecDamageDirection;
	CHandle< C_BaseEntity > m_hInflictor;
	CHandle< C_BaseEntity > m_hAttacker;
	CHandle< C_BaseEntity > m_hAbility;
	float32 m_flDamage;
	float32 m_flTotalledDamage;
	DamageTypes_t m_bitsDamageType;
	int32 m_iDamageCustom;
	AmmoIndex_t m_iAmmoType;
	float32 m_flOriginalDamage;
	bool m_bShouldBleed;
	bool m_bShouldSpark;
	TakeDamageFlags_t m_nDamageFlags;
	HitGroup_t m_iHitGroupId;
	int32 m_nNumObjectsPenetrated;
	float32 m_flFriendlyFireDamageReductionRatio;
	bool m_bStoppedBullet;
	CUtlLeanVector< DestructiblePartDamageRequest_t > m_DestructibleHitGroupRequests;
	// MNotSaved
	bool m_bInTakeDamageFlow;
};
