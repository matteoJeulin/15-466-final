#include "CameraBlock.hpp"
struct Level {
	/* Ranks */
    float s_rank_time = 0.0f;
	float a_rank_time = 0.0f;
	float b_rank_time = 0.0f;
	float c_rank_time = 0.0f;
	float d_rank_time = 0.0f;

	/* Spawn Location*/
    char spawnLocation[1024];

	/* Camera Blocks */
	int numberOfCameraBlocks;
	CameraBlock cameraBlocks[64];
};