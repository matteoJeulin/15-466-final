struct CameraBlock {
    // The player bounds for this block to be active
    float playerLeft = 0.0f;
    float playerRight = 0.0f;
    float playerBottom = 0.0f;
    float playerTop = 0.0f;

    // How far the camera is allowed to go while inside the block
    float cameraLeft = 0.0f;
    float cameraRight = 0.0f;
    float cameraBottom = 0.0f;
    float cameraTop = 0.0f;

    float cameraHorizontalOffset = 0.0f;
    float cameraVerticalOffset = 0.0f;
};