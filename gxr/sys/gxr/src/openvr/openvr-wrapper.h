#ifndef OPENVR_WRAPPER_H_
#define OPENVR_WRAPPER_H_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#include <openvr_capi.h>
#pragma clang diagnostic pop

#pragma GCC diagnostic pop


// missing from openvr_capi.h

// Global entry points
extern intptr_t VR_InitInternal( EVRInitError *peError, EVRApplicationType eType );
extern void VR_ShutdownInternal(void);
extern bool VR_IsHmdPresent(void);
extern intptr_t VR_GetGenericInterface( const char *pchInterfaceVersion, EVRInitError *peError );
extern bool VR_IsRuntimeInstalled(void);
extern const char * VR_GetVRInitErrorAsSymbol( EVRInitError error );
extern const char * VR_GetVRInitErrorAsEnglishDescription( EVRInitError error );

static inline uint64_t ButtonMaskFromId (EVRButtonId id)
{
  return 1ull << id;
}

#endif /* OPENVR_WRAPPER_H_ */
