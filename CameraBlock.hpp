// TODO: AoS vs SoA
struct CameraBlock {
    // The player bounds for this block to be active
    float playerLeft;
    float playerRight;
    float playerBottom;
    float playerTop;

    // How far the camera is allowed to go while inside the block
    float cameraLeft;
    float cameraRight;
    float cameraBottom;
    float cameraTop;
};