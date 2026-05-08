class CBaseAnimGraph : public C_BaseModelEntity
{
	// MSaveOpsForField = "GetAnimGraphControllerManagerSaveRestoreOps"
	CAnimGraphControllerManager m_graphControllerManager;
	// MSaveOpsForField = "GetAnimGraphControllerPtrSaveRestoreOps"
	CAnimGraphControllerBase* m_pMainGraphController;
	bool m_bInitiallyPopulateInterpHistory;
	bool m_bSuppressAnimEventSounds;
	bool m_bAnimGraphUpdateEnabled;
	// MNotSaved
	bool m_bAnimationUpdateScheduled;
	// MNotSaved
	Vector m_vecForce;
	// MNotSaved
	int32 m_nForceBone;
	// MNotSaved
	CBaseAnimGraph* m_pClientsideRagdoll;
	// MNotSaved
	bool m_bBuiltRagdoll;
	// MPhysPtr
	IPhysicsRagdollControl* m_pRagdollControl;
	PhysicsRagdollPose_t m_RagdollPose;
	bool m_bRagdollEnabled;
	// MNotSaved
	bool m_bRagdollClientSide;
	// MNotSaved
	bool m_bHasAnimatedMaterialAttributes;
};
