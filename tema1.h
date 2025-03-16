#include "components/simple_scene.h"
#include <vector>

namespace m1
{
    // Core Game Structures
    struct Tank {
        glm::vec2 position;     // x, y position
        float rotation;         // Tank rotation based on terrain
        float turretAngle;      // Angle of the turret
        bool isPlayer1;         // True for player 1, false for player 2
        float health;           // Tank health
        bool isActive;
    };

    struct Projectile {
        glm::vec2 position;     // Current position
        glm::vec2 velocity;     // Current velocity
        bool active = false;    // Whether projectile is in flight
        bool fromPlayer1;       // Which player fired it
    };

    struct Particle {
        glm::vec2 position;
        glm::vec2 velocity;
        float rotation;
        float rotationSpeed;
        float lifetime;
        float scale;
        glm::vec3 color;
        bool active = false;
    };

    struct Cloud {
        glm::vec2 position;
        float scale;
        float speed;
    };

    struct ExplosionEffect {
        std::vector<Particle> particles;
        glm::vec2 position;
        float duration;
        bool active = false;
    };

    class Tema1 : public gfxc::SimpleScene
    {
    public:
        Tema1();
        ~Tema1();
        void Init() override;

    private:
        // Constants - Game Parameters
        static const int TERRAIN_POINTS = 400;
        static const int PARTICLES_PER_EXPLOSION = 1000;
        static const int SMOKE_PARTICLES = 500;
        const float TERRAIN_WIDTH = 420.0f;
        const float TERRAIN_HEIGHT = 220.0f;
        const float TANK_SPEED = 15.0f;
        const float TURRET_ROTATION_SPEED = 1.0f;
        const float PROJECTILE_SPEED = 200.0f;
        const float GRAVITY = 200.0f;
        const float EXPLOSION_RADIUS = 20.0f;
        const float COLLISION_THRESHOLD = 5.0f;
        const float AI_SHOOTING_RANGE = 100.0f;

        // Particle System Constants
        const float PARTICLE_MAX_LIFETIME = 1.5f;
        const float PARTICLE_MIN_SPEED = 1.0f;
        const float PARTICLE_MAX_SPEED = 40.0f;
        const float SMOKE_MAX_LIFETIME = 0.6f;
        const float SMOKE_MIN_SPEED = 1.0f;
        const float SMOKE_MAX_SPEED = 40.0f;
        const float SMOKE_GRAVITY = 1.0f;

        // Terrain Function Parameters
        const float A1 = 1.0f, o1 = 1.0f;
        const float A2 = 2.0f, o2 = 0.5f;
        const float A3 = 0.5f, o3 = 3.0f;

        // Color Constants
        const glm::vec3 TERRAIN_COLOR_GREEN = glm::vec3(0.5f, 1.0f, 0.2f);
        const glm::vec3 TERRAIN_COLOR_SNOW = glm::vec3(1.0f, 1.0f, 1.0f);
        const glm::vec3 TERRAIN_COLOR_DESERT = glm::vec3(0.85f, 0.65f, 0.45f);
        const glm::vec3 TERRAIN_COLOR_GREEN_NIGHT = glm::vec3(0.2f, 0.3f, 0.2f);
        const glm::vec3 TERRAIN_COLOR_SNOW_NIGHT = glm::vec3(0.7f, 0.7f, 0.8f);
        const glm::vec3 TERRAIN_COLOR_DESERT_NIGHT = glm::vec3(0.6f, 0.5f, 0.3f);
        const glm::vec3 DAY_SKY_COLOR = glm::vec3(0.529f, 0.808f, 0.922f);
        const glm::vec3 NIGHT_SKY_COLOR = glm::vec3(0.1f, 0.15f, 0.3f);

        // Game State Variables
        float deltaTimeSeconds;
        float lastDeformationTime = 0.0f;
        glm::vec3 currentTerrainColor;
        bool isNightMode = false;
        bool aiModeEnabled1 = false;
        bool aiModeEnabled2 = false;
        bool needsSlideCheck = false;

        // Screen Shake Properties
        bool isShaking = false;
        float shakeDuration = 0.0f;
        float shakeTimeLeft = 0.0f;
        float shakeIntensity = 15.0f;
        glm::vec3 originalCameraPosition;

        // Game Objects
        Tank player1Tank;
        Tank player2Tank;
        std::vector<Projectile> projectiles;
        std::vector<ExplosionEffect> explosions;
        std::vector<Cloud> clouds;
        glm::mat3 modelMatrix;

        // Terrain Data
        std::vector<float> heightMap;
        std::vector<VertexFormat> vertices;
        std::vector<unsigned int> indices;

        // Mesh Objects
        Mesh* terrain;
        Mesh* tankBase;
        Mesh* tankTurret;
        Mesh* projectile;
        Mesh* aimLineMesh;
        Mesh* aimTrajectory;
        Mesh* particleMesh;
        Mesh* cloudMesh;
        Mesh* tankBase2;
        Mesh* tankTurret2;

        // Health Bar Meshes
        Mesh* healthBarRed;
        Mesh* healthBarGreen;
        Mesh* healthBarFrame;
        Mesh* healthBarOrange;
        Mesh* healthBarYellow;

        // Core Game Functions
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;

        // Initialization Functions
        void CreateTerrain();
        void GenerateHeightMap();
        void CreateTankMesh();
        void CreateProjectileMesh();
        void CreateHealthBarMesh();
        void CreateAimTrajectory();
        void CreateParticleMesh();
        void CreateCloudMesh();
        void InitializeClouds();

        // Terrain Functions
        float TerrainFunction(float x) const;
        float GetTerrainHeightAt(float x);
        float GetTerrainAngleAt(float x);
        void DeformTerrain(float x, float radius);
        void UpdateTerrainSlide(float deltaTimeSeconds);

        // Game Logic Functions
        void HandleInput(float deltaTime);
        void UpdateProjectiles(float deltaTime);
        void UpdateTankPositions();
        void UpdateClouds(float deltaTime);
        void UpdateExplosions(float deltaTimeSeconds);

        // Rendering Functions
        void RenderHealthBar(const Tank& tank);
        void RenderExplosions();

        // Effect Functions
        void CreateExplosion(const glm::vec2& position);
        void CreateGunSmoke(const glm::vec2& position, float angle);

        // Utility Functions
        void UpdateAimTrajectory(const Tank& tank);
        std::vector<glm::vec2> CalculateTrajectoryPoints(glm::vec2 startPos, glm::vec2 velocity, int numPoints);
    };
}
