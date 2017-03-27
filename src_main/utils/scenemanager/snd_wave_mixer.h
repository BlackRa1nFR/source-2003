//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef SND_WAVE_MIXER_H
#define SND_WAVE_MIXER_H
#pragma once

class CWaveData;
class CAudioMixer;

CAudioMixer *CreateWaveMixer( CWaveData *data, int format, int channels, int bits );


#endif // SND_WAVE_MIXER_H
