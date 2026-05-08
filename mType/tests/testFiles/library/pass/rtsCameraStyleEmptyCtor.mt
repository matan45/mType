// MYT-286 regression fixture — mirrors RTSCameraController.mt:
// @Script-annotated class, many fields with inline default initializers,
// empty user-defined constructor. The original constructor compiled to
// ~82 instructions; peephole then shrank that body, leaving the
// recorded instructionCount stale and overflowing the instructions[]
// array on disk so deserialize rejected the function.

@Script
class RTSCameraStyleScript {
    private int    selfId            = 0;
    private int    terrainId         = 0;
    private float  yaw               = 0.0;
    private float  pitch             = 0.0;
    private float  zoomLevel         = 1.0;
    private float  zoomSensitivity   = 0.1;
    private float  minHeight         = 1.0;
    private float  maxHeight         = 100.0;
    private float  panSpeed          = 5.0;
    private float  edgeThresholdPx   = 10.0;
    private float  rotateSensitivity = 0.05;
    private float  mapMinX           = -500.0;
    private float  mapMaxX           = 500.0;
    private float  mapMinZ           = -500.0;
    private float  mapMaxZ           = 500.0;
    private float  focalX            = 0.0;
    private float  focalZ            = 0.0;
    private float  lastTerrainY      = 0.0;
    private float  DEG_TO_RAD        = 0.017453;

    constructor() {
    }

    public function onStart(): void {
    }

    public function onUpdate(float dt): void {
    }

    public function onDestroy(): void {
    }

    private function applyTransform(): void {
    }
}
