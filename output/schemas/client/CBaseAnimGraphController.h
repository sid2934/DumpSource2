class CBaseAnimGraphController : public CSkeletonAnimationController
{
	AnimationAlgorithm_t m_nAnimationAlgorithm;
	ExternalAnimGraphHandle_t m_nNextExternalGraphHandle;
	C_NetworkUtlVectorBase< CGlobalSymbol > m_vecSecondarySkeletonSlotIDs;
	C_NetworkUtlVectorBase< CHandle< CBaseAnimGraph > > m_vecSecondarySkeletons;
	int32 m_nSecondarySkeletonMasterCount;
	float32 m_flSoundSyncTime;
	uint32 m_nActiveIKChainMask;
	HSequence m_hSequence;
	GameTime_t m_flSeqStartTime;
	float32 m_flSeqFixedCycle;
	AnimLoopMode_t m_nAnimLoopMode;
	CNetworkedQuantizedFloat m_flPlaybackRate;
	SequenceFinishNotifyState_t m_nNotifyState;
	bool m_bNetworkedAnimationInputsChanged;
	bool m_bNetworkedSequenceChanged;
	bool m_bLastUpdateSkipped;
	bool m_bSequenceFinished;
	GameTick_t m_nPrevAnimUpdateTick;
	CStrongHandle< InfoForResourceTypeCNmGraphDefinition > m_hGraphDefinitionAG2;
	// MNotSaved
	C_UtlVectorEmbeddedNetworkVar< AnimGraph2SerializedPoseRecipeSlot_t > m_SerializePoseRecipeAG2Slots;
	// MNotSaved
	C_NetworkUtlVectorBase< uint8 > m_SerializePoseRecipeAG2Dynamic;
	// MNotSaved
	uint32 m_nSerializePoseRecipeAG2ActiveSlot;
	// MNotSaved
	int32 m_nSerializePoseRecipeVersionAG2;
	int32 m_nServerGraphInstanceIteration;
	int32 m_nServerSerializationContextIteration;
	ResourceId_t m_primaryGraphId;
	C_NetworkUtlVectorBase< ResourceId_t > m_vecExternalGraphIds;
	C_NetworkUtlVectorBase< ResourceId_t > m_vecExternalClipIds;
	CGlobalSymbol m_sAnimGraph2Identifier;
	// MSaveOpsForField = "GetAnimGraph2SaveRestoreOps"
	CNmGraphInstance* m_pGraphInstanceAG2;
	// MSaveOpsForField = "GetExternalAnimGraphSaveRestoreOps"
	CUtlVector< ExternalAnimGraph_t > m_vecExternalGraphs;
	AnimationAlgorithm_t m_nPrevAnimationAlgorithm;
};
