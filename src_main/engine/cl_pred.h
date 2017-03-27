// cl_pred.h
#ifndef CL_PRED_H
#define CL_PRED_H
#ifdef _WIN32
#pragma once
#endif

typedef enum
{
	PREDICTION_SIMULATION_RESULTS_ARRIVING_ON_SEND_FRAME = 0,
	PREDICTION_NORMAL,
} PREDICTION_REASON;

void CL_RunPrediction( PREDICTION_REASON reason );
bool CL_HasRunPredictionThisFrame( void );
void CL_Predict( int world_start_state, bool validframe, int start_command, int stop_command );

#endif // CL_PRED_H