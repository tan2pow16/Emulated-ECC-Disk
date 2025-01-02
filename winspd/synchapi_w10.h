void InitializeSRWLock(void *SRWLock);
void AcquireSRWLockExclusive(void *SRWLock);
void ReleaseSRWLockExclusive(void *SRWLock);
void AcquireSRWLockShared(void *SRWLock);
void ReleaseSRWLockShared(void *SRWLock);
