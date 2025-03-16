#include "lab_m1/Tema1/Tema1.h"

using namespace std;
using namespace m1;

Tema1::Tema1()
{
}

Tema1::~Tema1()
{
}

void Tema1::Init()
{
    GetSceneCamera()->SetPosition(glm::vec3(210, 120, 200));
    GetSceneCamera()->SetRotation(glm::vec3(0, 0, 0));

    GenerateHeightMap();
    CreateTankMesh();
    CreateProjectileMesh();
    CreateHealthBarMesh();
    CreateAimTrajectory();
    CreateParticleMesh();
    CreateCloudMesh();
    InitializeClouds();
    currentTerrainColor = TERRAIN_COLOR_GREEN;
    CreateTerrain();


    // Initialize tank1
    player1Tank.position = glm::vec2(30, GetTerrainHeightAt(30));
    player1Tank.rotation = GetTerrainAngleAt(30);
    player1Tank.turretAngle = 0;
    player1Tank.isPlayer1 = true;
    player1Tank.health = 200;
    player1Tank.isActive = true;

    // Initialize tank2
    player2Tank.position = glm::vec2(TERRAIN_WIDTH - 30, GetTerrainHeightAt(TERRAIN_WIDTH - 30));
    player2Tank.rotation = GetTerrainAngleAt(TERRAIN_WIDTH - 30);
    player2Tank.turretAngle = -9.40f;
    player2Tank.isPlayer1 = false;
    player2Tank.health = 200;
    player2Tank.isActive = true;
}

void Tema1::FrameStart()
{
    if (isNightMode) {
        glClearColor(NIGHT_SKY_COLOR.r, NIGHT_SKY_COLOR.g, NIGHT_SKY_COLOR.b, 1.0f);
    }
    else {
        glClearColor(DAY_SKY_COLOR.r, DAY_SKY_COLOR.g, DAY_SKY_COLOR.b, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::ivec2 resolution = window->GetResolution();
    glViewport(0, 0, resolution.x, resolution.y);

    // cloud rendering
    for (const auto& cloud : clouds) {
        glm::mat3 modelMatrix = glm::mat3(1.0f);

        // Create a translation matrix directly for 2D
        glm::mat3 translationMatrix = glm::mat3(1.0f);
        translationMatrix[2][0] = cloud.position.x;
        translationMatrix[2][1] = cloud.position.y;

        // Create a scale matrix directly for 2D
        glm::mat3 scaleMatrix = glm::mat3(1.0f);
        scaleMatrix[0][0] = cloud.scale;
        scaleMatrix[1][1] = cloud.scale;

        modelMatrix = translationMatrix * scaleMatrix;

        RenderMesh2D(cloudMesh, shaders["VertexColor"], modelMatrix);
    }
}

void Tema1::Update(float deltaTimeSeconds)
{
    HandleInput(deltaTimeSeconds);
    UpdateProjectiles(deltaTimeSeconds);
    UpdateExplosions(deltaTimeSeconds);
    RenderExplosions();
    UpdateClouds(deltaTimeSeconds);

    // Render terrain
    RenderMesh(terrain, shaders["VertexColor"], glm::mat4(1));

    // Render Player 1 tank if active
    if (player1Tank.isActive) {
        UpdateAimTrajectory(player1Tank);
        RenderMesh(aimTrajectory, shaders["VertexColor"], glm::mat4(1));
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(player1Tank.position, 0));
        modelMatrix = glm::rotate(modelMatrix, player1Tank.rotation, glm::vec3(0, 0, 1));
        RenderMesh(tankBase, shaders["VertexColor"], modelMatrix);

        glm::mat4 turretModelMatrix = modelMatrix;
        turretModelMatrix = glm::translate(turretModelMatrix, glm::vec3(0.4f, 7.5f, 0));
        turretModelMatrix = glm::rotate(turretModelMatrix, player1Tank.turretAngle, glm::vec3(0, 0, 1));
        turretModelMatrix = glm::translate(turretModelMatrix, glm::vec3(0.4f, -7.5f, 0));
        RenderMesh(tankTurret, shaders["VertexColor"], turretModelMatrix);

        RenderHealthBar(player1Tank);
    }

    // Render Player 2 tank if active
    if (player2Tank.isActive) {
        UpdateAimTrajectory(player2Tank);
        RenderMesh(aimTrajectory, shaders["VertexColor"], glm::mat4(1));
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(player2Tank.position, 0));
        modelMatrix = glm::rotate(modelMatrix, player2Tank.rotation, glm::vec3(0, 0, 1));
        RenderMesh(tankBase2, shaders["VertexColor"], modelMatrix);

        glm::mat4 turretModelMatrix = modelMatrix;
        turretModelMatrix = glm::translate(turretModelMatrix, glm::vec3(0.4f, 7.5f, 0));
        turretModelMatrix = glm::rotate(turretModelMatrix, player2Tank.turretAngle, glm::vec3(0, 0, 1));
        turretModelMatrix = glm::translate(turretModelMatrix, glm::vec3(0.4f, -7.5f, 0));
        RenderMesh(tankTurret2, shaders["VertexColor"], turretModelMatrix);

        RenderHealthBar(player2Tank);
    }

    // shake screen effect
    if (shakeDuration > 0) {
        shakeDuration -= deltaTimeSeconds;

        // Generate random offsets for shake effect
        float offsetX = ((rand() % 300 - 100) / 100.0f) * shakeIntensity;
        float offsetY = ((rand() % 300 - 100) / 100.0f) * shakeIntensity;

        // Apply shake to camera
        glm::vec3 originalPosition = glm::vec3(210, 120, 200);
        GetSceneCamera()->SetPosition(originalPosition + glm::vec3(offsetX, offsetY, 0));

        // Reduce intensity gradually for damping effect
        shakeIntensity *= 0.9f;

        // Reset camera when shake duration ends
        if (shakeDuration <= 0) {
            GetSceneCamera()->SetPosition(originalPosition);
        }
    }

    // Ai shooting cooldown
    static float aiShootCooldown1 = 0.5f;
    static float aiShootCooldown2 = 0.5f;

    if (aiModeEnabled1 && player1Tank.isActive && player2Tank.isActive) {
        // Calculate the distance between player1Tank and player2Tank
        float distanceToPlayer2 = glm::length(player2Tank.position - player1Tank.position);

        // Move player1Tank towards player2Tank if not within shooting range
        if (distanceToPlayer2 > AI_SHOOTING_RANGE) {
            float direction = player2Tank.position.x > player1Tank.position.x ? 1.0f : -1.0f;
            player1Tank.position.x += direction * TANK_SPEED * deltaTimeSeconds;
            player1Tank.position.y = GetTerrainHeightAt(player1Tank.position.x);
            player1Tank.rotation = GetTerrainAngleAt(player1Tank.position.x);
        }

        // Aim turret towards player2Tank if within shooting range
        if (distanceToPlayer2 <= AI_SHOOTING_RANGE) {
            glm::vec2 directionToPlayer2 = glm::normalize(player2Tank.position - player1Tank.position);
            player1Tank.turretAngle = atan2(directionToPlayer2.y, directionToPlayer2.x) - player1Tank.rotation;

            // AI shooting cooldown logic
            aiShootCooldown1 -= deltaTimeSeconds;
            if (aiShootCooldown1 <= 0) {
                Projectile p;
                p.position = glm::vec2(
                    player1Tank.position.x + 20 * cos(player1Tank.rotation + player1Tank.turretAngle),
                    player1Tank.position.y + 6 + 20 * sin(player1Tank.rotation + player1Tank.turretAngle)
                );
                p.velocity = glm::vec2(
                    PROJECTILE_SPEED * cos(player1Tank.rotation + player1Tank.turretAngle),
                    PROJECTILE_SPEED * sin(player1Tank.rotation + player1Tank.turretAngle)
                );
                p.active = true;
                p.fromPlayer1 = true;
                projectiles.push_back(p);
                aiShootCooldown1 = 0.5f;

                // Generate gun smoke effect
                CreateGunSmoke(p.position, player1Tank.rotation + player1Tank.turretAngle);
            }
        }
    }

    if (aiModeEnabled2 && player1Tank.isActive && player2Tank.isActive) {
        // Calculate the distance between player1Tank and player2Tank
        float distanceToPlayer1 = glm::length(player1Tank.position - player2Tank.position);

        // Move tank2 towards player1Tank if not within shooting range
        if (distanceToPlayer1 > AI_SHOOTING_RANGE) {
            float direction = player1Tank.position.x > player2Tank.position.x ? 1.0f : -1.0f;
            player2Tank.position.x += direction * TANK_SPEED * deltaTimeSeconds;
            player2Tank.position.y = GetTerrainHeightAt(player2Tank.position.x);
            player2Tank.rotation = GetTerrainAngleAt(player2Tank.position.x);
        }

        // Aim turret towards player1Tank if within shooting range
        if (distanceToPlayer1 <= AI_SHOOTING_RANGE) {
            glm::vec2 directionToPlayer1 = glm::normalize(player1Tank.position - player2Tank.position);
            player2Tank.turretAngle = atan2(directionToPlayer1.y, directionToPlayer1.x) - player2Tank.rotation;

            // AI shooting cooldown logic
            aiShootCooldown2 -= deltaTimeSeconds;
            if (aiShootCooldown2 <= 0) {
                Projectile p;
                p.position = glm::vec2(
                    player2Tank.position.x + 20 * cos(player2Tank.rotation + player2Tank.turretAngle),
                    player2Tank.position.y + 6 + 20 * sin(player2Tank.rotation + player2Tank.turretAngle)
                );
                p.velocity = glm::vec2(
                    PROJECTILE_SPEED * cos(player2Tank.rotation + player2Tank.turretAngle),
                    PROJECTILE_SPEED * sin(player2Tank.rotation + player2Tank.turretAngle)
                );
                p.active = true;
                p.fromPlayer1 = false;
                projectiles.push_back(p);
                aiShootCooldown2 = 0.5f;

                // Generate gun smoke effect
                CreateGunSmoke(p.position, player2Tank.rotation + player2Tank.turretAngle);
            }
        }
    }

    if (needsSlideCheck) {
        lastDeformationTime += deltaTimeSeconds;
        // Start sliding animation after a small delay
        if (lastDeformationTime >= 0.1f) {
            UpdateTerrainSlide(deltaTimeSeconds);
        }
    }

    // Render active projectiles
    for (const auto& p : projectiles) {
        if (p.active) {
            glm::mat4 modelMatrix = glm::mat4(1);
            modelMatrix = glm::translate(modelMatrix, glm::vec3(p.position, 0));
            RenderMesh(projectile, shaders["VertexColor"], modelMatrix);
        }
    }
}

void Tema1::FrameEnd()
{
}

void Tema1::HandleInput(float deltaTime)
{
    // Player 1 controls (A/D for movement, W/S for turret, Space to shoot)
    if (!aiModeEnabled1) {
        if (window->KeyHold(GLFW_KEY_A)) {
            player1Tank.position.x -= TANK_SPEED * deltaTime;
            player1Tank.position.y = GetTerrainHeightAt(player1Tank.position.x);
            player1Tank.rotation = GetTerrainAngleAt(player1Tank.position.x);
        }
        if (window->KeyHold(GLFW_KEY_D)) {
            player1Tank.position.x += TANK_SPEED * deltaTime;
            player1Tank.position.y = GetTerrainHeightAt(player1Tank.position.x);
            player1Tank.rotation = GetTerrainAngleAt(player1Tank.position.x);
        }
        if (window->KeyHold(GLFW_KEY_W)) {
            player1Tank.turretAngle += TURRET_ROTATION_SPEED * deltaTime;
        }
        if (window->KeyHold(GLFW_KEY_S)) {
            player1Tank.turretAngle -= TURRET_ROTATION_SPEED * deltaTime;
        }
    }

    // Player 2 controls (Left/Right for movement, Up/Down for turret, Enter to shoot)
    if (!aiModeEnabled2) {
        if (window->KeyHold(GLFW_KEY_LEFT)) {
            player2Tank.position.x -= TANK_SPEED * deltaTime;
            player2Tank.position.y = GetTerrainHeightAt(player2Tank.position.x);
            player2Tank.rotation = GetTerrainAngleAt(player2Tank.position.x);
        }
        if (window->KeyHold(GLFW_KEY_RIGHT)) {
            player2Tank.position.x += TANK_SPEED * deltaTime;
            player2Tank.position.y = GetTerrainHeightAt(player2Tank.position.x);
            player2Tank.rotation = GetTerrainAngleAt(player2Tank.position.x);
        }
        if (window->KeyHold(GLFW_KEY_DOWN)) {
            player2Tank.turretAngle += TURRET_ROTATION_SPEED * deltaTime;
        }
        if (window->KeyHold(GLFW_KEY_UP)) {
            player2Tank.turretAngle -= TURRET_ROTATION_SPEED * deltaTime;
        }
    }

    if (window->KeyHold(GLFW_KEY_1)) { // plain map (default)
        if (currentTerrainColor != TERRAIN_COLOR_GREEN) {
            currentTerrainColor = TERRAIN_COLOR_GREEN;
            CreateTerrain();
        }
    }
    if (window->KeyHold(GLFW_KEY_2)) { // snowy map
        if (currentTerrainColor != TERRAIN_COLOR_SNOW) {
            currentTerrainColor = TERRAIN_COLOR_SNOW;
            CreateTerrain();
        }
    }
    if (window->KeyHold(GLFW_KEY_3)) { // desert map
        if (currentTerrainColor != TERRAIN_COLOR_DESERT) {
            currentTerrainColor = TERRAIN_COLOR_DESERT;
            CreateTerrain();
        }
    }

    if (window->KeyHold(GLFW_KEY_4)) {
        if (isNightMode) {  // Only update if actually changing modes
            isNightMode = false;
            CreateTerrain();  // Recreate terrain with day colors
        }
    }
    if (window->KeyHold(GLFW_KEY_5)) {
        if (!isNightMode) {  // Only update if actually changing modes
            isNightMode = true;
            CreateTerrain();  // Recreate terrain with night colors
        }
    }

    if (window->KeyHold(GLFW_KEY_6)) { // deactivate ai mode for tank1
        aiModeEnabled1 = false;
    }
    if (window->KeyHold(GLFW_KEY_7)) { // activate ai mode for tank1
        aiModeEnabled1 = true;
    }
    if (window->KeyHold(GLFW_KEY_8)) { // deactivate ai mode for tank2
        aiModeEnabled2 = false;
    }
    if (window->KeyHold(GLFW_KEY_9)) { // activate ai mode for tank2
        aiModeEnabled2 = true;
    }

    // Shooting
    static float shootCooldown1 = 0;
    static float shootCooldown2 = 0;

    shootCooldown1 -= deltaTime;
    shootCooldown2 -= deltaTime;

    if (!aiModeEnabled1) {
        if (window->KeyHold(GLFW_KEY_SPACE) && shootCooldown1 <= 0 && player1Tank.isActive) {
            Projectile p;
            p.position = glm::vec2(
                player1Tank.position.x + 20 * cos(player1Tank.rotation + player1Tank.turretAngle),
                player1Tank.position.y + 6 + 20 * sin(player1Tank.rotation + player1Tank.turretAngle)
            );
            p.velocity = glm::vec2(
                PROJECTILE_SPEED * cos(player1Tank.rotation + player1Tank.turretAngle),
                PROJECTILE_SPEED * sin(player1Tank.rotation + player1Tank.turretAngle)
            );
            p.active = true;
            p.fromPlayer1 = true;
            projectiles.push_back(p);
            shootCooldown1 = 0.3f;

            // Gun smoke for tank1
            float shotAngle = player1Tank.rotation + player1Tank.turretAngle;
            glm::vec2 muzzlePos(
                player1Tank.position.x + 20 * cos(shotAngle),
                player1Tank.position.y + 6 + 20 * sin(shotAngle)
            );

            p.position = muzzlePos;
            p.velocity = glm::vec2(
                PROJECTILE_SPEED * cos(shotAngle),
                PROJECTILE_SPEED * sin(shotAngle)
            );
            CreateGunSmoke(muzzlePos, shotAngle);
        }
    }

    if (!aiModeEnabled2) {
        if (window->KeyHold(GLFW_KEY_ENTER) && shootCooldown2 <= 0 && player2Tank.isActive) {
            Projectile p;
            p.position = glm::vec2(
                player2Tank.position.x + 20 * cos(player2Tank.rotation + player2Tank.turretAngle),
                player2Tank.position.y + 6 + 20 * sin(player2Tank.rotation + player2Tank.turretAngle)
            );
            p.velocity = glm::vec2(
                PROJECTILE_SPEED * cos(player2Tank.rotation + player2Tank.turretAngle),
                PROJECTILE_SPEED * sin(player2Tank.rotation + player2Tank.turretAngle)
            );
            p.active = true;
            p.fromPlayer1 = false;
            projectiles.push_back(p);
            shootCooldown2 = 0.3f;

            // Gun smoke for tank2
            float shotAngle = player2Tank.rotation + player2Tank.turretAngle;
            glm::vec2 muzzlePos(
                player2Tank.position.x + 20 * cos(shotAngle),
                player2Tank.position.y + 6 + 20 * sin(shotAngle)
            );

            p.position = muzzlePos;
            p.velocity = glm::vec2(
                PROJECTILE_SPEED * cos(shotAngle),
                PROJECTILE_SPEED * sin(shotAngle)
            );
            CreateGunSmoke(muzzlePos, shotAngle);

        }
    }
}

/* ||---------------------------------------------------------------------------------------------------------------------------------------------------|| */
/* ||--------------------------------------------------------------- basic functionalities -------------------------------------------------------------|| */
/* ||---------------------------------------------------------------------------------------------------------------------------------------------------|| */

float Tema1::TerrainFunction(float x) const
{
    // Convert x to normalized range [0, 2π] for sine functions
    float normalized_x = (x / TERRAIN_WIDTH) * 2 * M_PI;
    return (A1 * sin(o1 * normalized_x) +            // First wave component
        A2 * sin(o2 * normalized_x) +                // Second wave component
        A3 * sin(o3 * normalized_x)) * 8.0f + 75.0f; // Third wave component + baseline height
}

float Tema1::GetTerrainHeightAt(float x)
{
    if (x < 0) return heightMap[0];
    if (x > TERRAIN_WIDTH) return heightMap[static_cast<std::vector<float, std::allocator<float>>::size_type>(TERRAIN_POINTS) - 1];

    float step = TERRAIN_WIDTH / (TERRAIN_POINTS - 1);
    int index = (int)(x / step);

    // Calculate interpolation factor
    float t = (x - index * step) / step;

    if (index >= TERRAIN_POINTS - 1) return heightMap[static_cast<std::vector<float, std::allocator<float>>::size_type>(TERRAIN_POINTS) - 1];

    // Linear interpolation between adjacent height points
    return heightMap[index] * (1 - t) + heightMap[static_cast<std::vector<float, std::allocator<float>>::size_type>(index) + 1] * t;
}

float Tema1::GetTerrainAngleAt(float x)
{
    float step = TERRAIN_WIDTH / (TERRAIN_POINTS - 1);
    float h1 = GetTerrainHeightAt(x - step); // Height at previous point
    float h2 = GetTerrainHeightAt(x + step); // Height at next point
    return atan2(h2 - h1, 2 * step);         // Calculate angle using arctan
}

void Tema1::GenerateHeightMap()
{
    heightMap.clear();
    float step = TERRAIN_WIDTH / (TERRAIN_POINTS - 1);

    for (int i = 0; i < TERRAIN_POINTS; i++) {
        float x = i * step * 3; // Multiply by 3 to stretch out the terrain
        heightMap.push_back(TerrainFunction(x));
    }
}

void Tema1::CreateTerrain()
{
    vertices.clear();
    indices.clear();

    // Determine the correct terrain color based on current mode
    glm::vec3 terrainColor;
    if (isNightMode) {
        if (currentTerrainColor == TERRAIN_COLOR_GREEN) {
            terrainColor = TERRAIN_COLOR_GREEN_NIGHT;
        }
        else if (currentTerrainColor == TERRAIN_COLOR_SNOW) {
            terrainColor = TERRAIN_COLOR_SNOW_NIGHT;
        }
        else if (currentTerrainColor == TERRAIN_COLOR_DESERT) {
            terrainColor = TERRAIN_COLOR_DESERT_NIGHT;
        }
        else {
            terrainColor = currentTerrainColor;
        }
    }
    else {
        terrainColor = currentTerrainColor;
    }

    float step = TERRAIN_WIDTH / (TERRAIN_POINTS - 1);

    // Generate vertices for triangle strip
    for (int i = 0; i < TERRAIN_POINTS - 1; i++) { 
        // Bottom vertex
        vertices.push_back(VertexFormat(
            glm::vec3(i * step, 0, 0),
            terrainColor
        ));

        // Top vertex
        vertices.push_back(VertexFormat(
            glm::vec3(i * step, heightMap[i], 0),
            terrainColor
        ));
    }

    // Add final vertices at the end of terrain
    vertices.push_back(VertexFormat(
        glm::vec3(TERRAIN_WIDTH, 0, 0),
        terrainColor
    ));
    vertices.push_back(VertexFormat(
        glm::vec3(TERRAIN_WIDTH, heightMap[TERRAIN_POINTS - 1], 0),
        terrainColor
    ));

    // Create sequential indices for triangle strip
    for (unsigned int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }

    terrain = new Mesh("terrain");
    terrain->SetDrawMode(GL_TRIANGLE_STRIP);
    terrain->InitFromData(vertices, indices);
}

void Tema1::CreateTankMesh()
{
    // Player 1 Tank
    {
        vector<VertexFormat> tankVertices;
        vector<unsigned int> tankIndices;

        // First trapezoid (upper - larger)
        tankVertices.push_back(VertexFormat(glm::vec3(-12, 9, 0), glm::vec3(0.48f, 0.32f, 0.18f)));  // Top left
        tankVertices.push_back(VertexFormat(glm::vec3(12, 9, 0), glm::vec3(0.48f, 0.32f, 0.18f)));   // Top right
        tankVertices.push_back(VertexFormat(glm::vec3(15, 2, 0), glm::vec3(0.48f, 0.32f, 0.18f)));   // Bottom right
        tankVertices.push_back(VertexFormat(glm::vec3(-15, 2, 0), glm::vec3(0.48f, 0.32f, 0.18f)));  // Bottom left

        tankIndices = { 0, 1, 2, 0, 2, 3 };

        // Second trapezoid (inverted - smaller)
        tankVertices.push_back(VertexFormat(glm::vec3(-11, 2, 0), glm::vec3(0, 0, 0)));  // Top left
        tankVertices.push_back(VertexFormat(glm::vec3(11, 2, 0), glm::vec3(0, 0, 0)));   // Top right
        tankVertices.push_back(VertexFormat(glm::vec3(8, -2, 0), glm::vec3(0, 0, 0)));   // Bottom right
        tankVertices.push_back(VertexFormat(glm::vec3(-8, -2, 0), glm::vec3(0, 0, 0)));  // Bottom left

        tankIndices.insert(tankIndices.end(), { 4, 5, 6, 4, 6, 7 });

        tankBase = new Mesh("tank_base_player1");
        tankBase->SetDrawMode(GL_TRIANGLES);
        tankBase->InitFromData(tankVertices, tankIndices);
    }

    // Player 2 Tank
    {
        vector<VertexFormat> tankVertices;
        vector<unsigned int> tankIndices;

        // First trapezoid (upper - larger)
        tankVertices.push_back(VertexFormat(glm::vec3(-12, 9, 0), glm::vec3(0.2, 0.4, 0.2)));  // Top left
        tankVertices.push_back(VertexFormat(glm::vec3(12, 9, 0), glm::vec3(0.2, 0.4, 0.2)));   // Top right
        tankVertices.push_back(VertexFormat(glm::vec3(15, 2, 0), glm::vec3(0.2, 0.4, 0.2)));   // Bottom right
        tankVertices.push_back(VertexFormat(glm::vec3(-15, 2, 0), glm::vec3(0.2, 0.4, 0.2)));  // Bottom left

        tankIndices = { 0, 1, 2, 0, 2, 3 };

        // Second trapezoid (inverted - smaller)
        tankVertices.push_back(VertexFormat(glm::vec3(-11, 2, 0), glm::vec3(0, 0, 0)));  // Top left
        tankVertices.push_back(VertexFormat(glm::vec3(11, 2, 0), glm::vec3(0, 0, 0)));   // Top right
        tankVertices.push_back(VertexFormat(glm::vec3(8, -2, 0), glm::vec3(0, 0, 0)));   // Bottom right
        tankVertices.push_back(VertexFormat(glm::vec3(-8, -2, 0), glm::vec3(0, 0, 0)));  // Bottom left

        tankIndices.insert(tankIndices.end(), { 4, 5, 6, 4, 6, 7 });

        tankBase2 = new Mesh("tank_base_player2");
        tankBase2->SetDrawMode(GL_TRIANGLES);
        tankBase2->InitFromData(tankVertices, tankIndices);
    }

    // Player 1 Turret
    {
        vector<VertexFormat> turretVertices;
        vector<unsigned int> turretIndices;

        const int segments = 40;
        float turretRadius = 6.0f;
        float turretYOffset = 7.0f;

        // Center point
        turretVertices.push_back(VertexFormat(glm::vec3(0, turretYOffset, 0), glm::vec3(0.48f, 0.32f, 0.18f)));

        // Circular turret perimeter
        for (int i = 0; i <= segments; i++) {
            float angle = 2 * M_PI * i / segments;
            float x = turretRadius * cos(angle);
            float y = turretRadius * sin(angle) + turretYOffset;
            turretVertices.push_back(VertexFormat(glm::vec3(x, y, 0), glm::vec3(0.48f, 0.32f, 0.18f)));
        }

        for (int i = 1; i <= segments; i++) {
            turretIndices.push_back(0);
            turretIndices.push_back(i);
            turretIndices.push_back(i + 1);
        }

        // Cannon
        float cannonLength = 18.0f;
        float cannonWidth = 1.0f;

        turretVertices.push_back(VertexFormat(glm::vec3(0, cannonWidth + turretYOffset, 0), glm::vec3(0.6f, 0.4f, 0.12f)));
        turretVertices.push_back(VertexFormat(glm::vec3(cannonLength, cannonWidth + turretYOffset, 0), glm::vec3(0.6f, 0.4f, 0.12f)));
        turretVertices.push_back(VertexFormat(glm::vec3(cannonLength, -cannonWidth + turretYOffset, 0), glm::vec3(0.6f, 0.4f, 0.12f)));
        turretVertices.push_back(VertexFormat(glm::vec3(0, -cannonWidth + turretYOffset, 0), glm::vec3(0.6f, 0.4f, 0.12f)));

        int baseIndex = turretVertices.size() - 4;
        turretIndices.push_back(baseIndex);
        turretIndices.push_back(baseIndex + 1);
        turretIndices.push_back(baseIndex + 2);
        turretIndices.push_back(baseIndex);
        turretIndices.push_back(baseIndex + 2);
        turretIndices.push_back(baseIndex + 3);

        tankTurret = new Mesh("tank_turret_player1");
        tankTurret->SetDrawMode(GL_TRIANGLES);
        tankTurret->InitFromData(turretVertices, turretIndices);
    }

    // Player 2 Turret
    {
        vector<VertexFormat> turretVertices;
        vector<unsigned int> turretIndices;

        const int segments = 40;
        float turretRadius = 6.0f;
        float turretYOffset = 7.0f;

        // Center point
        turretVertices.push_back(VertexFormat(glm::vec3(0, turretYOffset, 0), glm::vec3(0.2, 0.4, 0.2)));

        // Circular turret perimeter
        for (int i = 0; i <= segments; i++) {
            float angle = 2 * M_PI * i / segments;
            float x = turretRadius * cos(angle);
            float y = turretRadius * sin(angle) + turretYOffset;
            turretVertices.push_back(VertexFormat(glm::vec3(x, y, 0), glm::vec3(0.2, 0.4, 0.2)));
        }

        for (int i = 1; i <= segments; i++) {
            turretIndices.push_back(0);
            turretIndices.push_back(i);
            turretIndices.push_back(i + 1);
        }

        // Cannon
        float cannonLength = 18.0f;
        float cannonWidth = 1.0f;

        turretVertices.push_back(VertexFormat(glm::vec3(0, cannonWidth + turretYOffset, 0), glm::vec3(0.25f, 0.35f, 0.16f)));
        turretVertices.push_back(VertexFormat(glm::vec3(cannonLength, cannonWidth + turretYOffset, 0), glm::vec3(0.25f, 0.35f, 0.16f)));
        turretVertices.push_back(VertexFormat(glm::vec3(cannonLength, -cannonWidth + turretYOffset, 0), glm::vec3(0.25f, 0.35f, 0.16f)));
        turretVertices.push_back(VertexFormat(glm::vec3(0, -cannonWidth + turretYOffset, 0), glm::vec3(0.25f, 0.35f, 0.16f)));

        int baseIndex = turretVertices.size() - 4;
        turretIndices.push_back(baseIndex);
        turretIndices.push_back(baseIndex + 1);
        turretIndices.push_back(baseIndex + 2);
        turretIndices.push_back(baseIndex);
        turretIndices.push_back(baseIndex + 2);
        turretIndices.push_back(baseIndex + 3);

        tankTurret2 = new Mesh("tank_turret_player2");
        tankTurret2->SetDrawMode(GL_TRIANGLES);
        tankTurret2->InitFromData(turretVertices, turretIndices);
    }
}

void Tema1::UpdateTankPositions() {
    // Update player 1 tank position
    player1Tank.position.y = GetTerrainHeightAt(player1Tank.position.x);
    player1Tank.rotation = GetTerrainAngleAt(player1Tank.position.x);

    // Update player 2 tank position
    player2Tank.position.y = GetTerrainHeightAt(player2Tank.position.x);
    player2Tank.rotation = GetTerrainAngleAt(player2Tank.position.x);
}

void Tema1::UpdateProjectiles(float deltaTimeSeconds)
{
    for (auto& p : projectiles) {
        if (!p.active) continue;

        // Update position
        p.position += p.velocity * deltaTimeSeconds;
        // Apply gravity
        p.velocity.y -= GRAVITY * deltaTimeSeconds;

        // Check terrain collision
        float terrainHeight = GetTerrainHeightAt(p.position.x);
        if (p.position.y <= terrainHeight) {
            float tankRadius = 15.0f;
            bool hitTank = false;

            // Check for Player tank 1 shot hits tank 2
            float dist2 = glm::length(p.position - player2Tank.position);
            if (dist2 < tankRadius && p.fromPlayer1 && player2Tank.isActive) {
                hitTank = true;
                // Increased base damage and made it more significant
                float damage = 25.0f * (1.0f - (dist2 / tankRadius));
                player2Tank.health -= damage; // Direct subtraction
                player2Tank.health = std::max(0.0f, player2Tank.health);

                if (player2Tank.health <= 0) {
                    player2Tank.isActive = false;
                    CreateExplosion(player2Tank.position);
                    shakeDuration = 3.0f;
                    shakeIntensity = 25.0f;
                }
                p.active = false;
            }

            // Check for Player tank 2 shot hits tank 1
            float dist1 = glm::length(p.position - player1Tank.position);
            if (dist1 < tankRadius && !p.fromPlayer1 && player1Tank.isActive) {
                hitTank = true;
                // Increased base damage and made it more significant
                float damage = 25.0f * (1.0f - (dist1 / tankRadius));
                player1Tank.health -= damage;
                player1Tank.health = std::max(0.0f, player1Tank.health);

                if (player1Tank.health <= 0) {
                    player1Tank.isActive = false;
                    CreateExplosion(player1Tank.position);
                    shakeDuration = 3.0f;
                    shakeIntensity = 25.0f;
                }
                p.active = false;
            }

            // If we didnt hit a tank deform the terrain
            if (!hitTank) {
                DeformTerrain(p.position.x, EXPLOSION_RADIUS);
                UpdateTankPositions();
                p.active = false;
            }
        }

        // Remove if out of bounds
        if (p.position.x < 0 || p.position.x > TERRAIN_WIDTH || p.position.y < 0) {
            p.active = false;
        }
    }

    // Remove inactive projectiles (memory management)
    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
            [](const Projectile& p) { return !p.active; }),
        projectiles.end()
    );
}

void Tema1::CreateProjectileMesh()
{
    vector<VertexFormat> projectileVertices;
    vector<unsigned int> projectileIndices;

    // Create circle
    const int segments = 12;
    projectileVertices.push_back(VertexFormat(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)));

    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = 2 * cos(angle);
        float y = 2 * sin(angle);
        projectileVertices.push_back(VertexFormat(glm::vec3(x, y, 0), glm::vec3(0, 0, 0)));
    }

    for (int i = 0; i < segments; i++) {
        projectileIndices.push_back(0);
        projectileIndices.push_back(1 + i);
        projectileIndices.push_back(1 + ((i + 1) % segments));
    }

    projectile = new Mesh("projectile");
    projectile->SetDrawMode(GL_TRIANGLES);
    projectile->InitFromData(projectileVertices, projectileIndices);
}

/* ||---------------------------------------------------------------------------------------------------------------------------------------------------|| */
/* ||--------------------------------------------------------- advance and bonus functionalities -------------------------------------------------------|| */
/* ||---------------------------------------------------------------------------------------------------------------------------------------------------|| */

void Tema1::CreateAimTrajectory()
{
    vector<VertexFormat> vertices;
    vector<unsigned int> indices;

    // Create initial empty trajectory line
    for (int i = 0; i < 3000; i++) {
        vertices.push_back(VertexFormat(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1))); // White color
        indices.push_back(i);
    }

    aimTrajectory = new Mesh("aim_trajectory");
    aimTrajectory->SetDrawMode(GL_LINE_STRIP);
    aimTrajectory->InitFromData(vertices, indices);
}

std::vector<glm::vec2> Tema1::CalculateTrajectoryPoints(glm::vec2 startPos, glm::vec2 velocity, int numPoints)
{
    std::vector<glm::vec2> points;
    float dt = 0.001f; // high precision time step for trajectory prediction

    glm::vec2 pos = startPos;
    glm::vec2 vel = velocity;

    for (int i = 0; i < numPoints; i++) {
        points.push_back(pos);

        // Update position and velocity using physics equations
        pos += vel * dt;
        vel.y -= GRAVITY * dt;

        // Check for terrain collision
        if (pos.x >= 0 && pos.x <= TERRAIN_WIDTH) {
            float terrainHeight = GetTerrainHeightAt(pos.x);
            if (pos.y <= terrainHeight) {
                break;
            }
        }

        // Break if out of bounds
        if (pos.x < 0 || pos.x > TERRAIN_WIDTH || pos.y < 0) {
            break;
        }
    }

    return points;
}

void Tema1::UpdateAimTrajectory(const Tank& tank)
{
    // Calculate start position from tank gun
    glm::vec2 startPos(
        tank.position.x + 20 * cos(tank.rotation + tank.turretAngle),
        tank.position.y + 8 + 20 * sin(tank.rotation + tank.turretAngle)
    );
    // Calculate initial velocity
    glm::vec2 velocity(
        PROJECTILE_SPEED * cos(tank.rotation + tank.turretAngle),
        PROJECTILE_SPEED * sin(tank.rotation + tank.turretAngle)
    );
    // Calculate trajectory points with high precision
    auto points = CalculateTrajectoryPoints(startPos, velocity, 3000);

    vector<VertexFormat> vertices;
    for (const auto& point : points) {
        vertices.push_back(VertexFormat(glm::vec3(point.x, point.y, 0), glm::vec3(1, 1, 1)));
    }

    vector<unsigned int> indices(vertices.size());
    for (size_t i = 0; i < indices.size(); i++) {
        indices[i] = i;
    }

    aimTrajectory->InitFromData(vertices, indices);
}

void Tema1::UpdateTerrainSlide(float deltaTimeSeconds) {
    const float HEIGHT_DIFFERENCE_THRESHOLD = 20.0f;
    const float SLIDE_RATE = 0.01f;
    const int AFFECTED_POINTS = 40;
    bool anySlideOccurred = false;

    // Temporary buffer to store height changes
    std::vector<float> heightChanges(heightMap.size(), 0.0f);

    // Find sharp height differences within a 10-point window
    for (size_t i = 5; i < heightMap.size() - 5; i++) {
        float forwardDiff = 0;
        float backwardDiff = 0;

        // Check height differences 10 points ahead and behind
        if (i + 10 < heightMap.size()) {
            forwardDiff = abs(heightMap[i] - heightMap[i + 10]);
        }
        if (i >= 10) {
            backwardDiff = abs(heightMap[i] - heightMap[i - 10]);
        }

        // If a sharp difference is found, smooth only the local area
        if (forwardDiff > HEIGHT_DIFFERENCE_THRESHOLD || backwardDiff > HEIGHT_DIFFERENCE_THRESHOLD) {
            // Calculate the range to smooth
            int startIdx = max(0, (int)i - AFFECTED_POINTS);
            int endIdx = min((int)heightMap.size() - 1, (int)i + AFFECTED_POINTS);

            // Calculate target heights for affected points
            for (int idx = startIdx; idx <= endIdx; idx++) {
                // Calculate distance from center point
                float distanceFromCenter = abs(idx - (float)i);

                // Gaussian falloff based on distance
                float weight = exp(-(distanceFromCenter * distanceFromCenter) / (2 * AFFECTED_POINTS));
                float transferAmount = SLIDE_RATE * deltaTimeSeconds * weight;

                // Calculate target height using weighted average of nearby points
                float targetHeight = 0.0f;
                float totalWeight = 0.0f;

                // Use 5 points on each side for averaging
                for (int j = -5; j <= 5; j++) {
                    int avgIdx = idx + j;
                    if (avgIdx < startIdx || avgIdx > endIdx) continue;

                    float pointWeight = exp(-(j * j) / 10.0f);
                    targetHeight += heightMap[avgIdx] * pointWeight;
                    totalWeight += pointWeight;
                }

                targetHeight /= totalWeight;

                // Calculate height change
                heightChanges[idx] += (targetHeight - heightMap[idx]) * transferAmount;
            }

            anySlideOccurred = true;
        }
    }

    // Apply changes only where needed
    if (anySlideOccurred) {
        for (size_t i = 0; i < heightMap.size(); i++) {
            if (heightChanges[i] != 0) {
                heightMap[i] += heightChanges[i];

                // Local smoothing to prevent new sharp points
                if (i > 0 && i < heightMap.size() - 1) {
                    float localAverage = (heightMap[i - 1] + heightMap[i + 1]) / 2.0f;
                    heightMap[i] = heightMap[i] * 0.8f + localAverage * 0.2f;
                }
            }
        }
        CreateTerrain();
        UpdateTankPositions();
    }
    else {
        needsSlideCheck = false;
    }
}

void Tema1::DeformTerrain(float x, float radius) {
    const float MIN_HEIGHT = 3.0f;
    float step = TERRAIN_WIDTH / (TERRAIN_POINTS - 1);
    int centerIndex = (int)(x / step);
    int radiusPoints = (int)(radius / step);

    // Store original heights for smooth transition
    std::vector<float> originalHeights = heightMap;

    // Apply explosion deformation
    for (int i = max(0, centerIndex - radiusPoints);
        i < min(TERRAIN_POINTS, centerIndex + radiusPoints); i++) {
        float distance = abs(i * step - x);
        if (distance < radius) {
            // Smooth falloff using quadratic easing
            float falloff = 1.0f - (distance * distance) / (radius * radius);
            float deformation = radius * falloff;

            // Apply deformation with minimum height constraint
            float newHeight = heightMap[i] - deformation;
            heightMap[i] = max(newHeight, MIN_HEIGHT);
        }
    }

    // Trigger terrain slide check after deformation
    needsSlideCheck = true;
    lastDeformationTime = 0.0f;

    CreateTerrain();
}

void Tema1::CreateHealthBarMesh()
{
    vector<VertexFormat> vertices;
    vector<unsigned int> indices;
    float frameThickness = 1.0f;

    vertices.push_back(VertexFormat(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0)));  // Bottom left
    vertices.push_back(VertexFormat(glm::vec3(30, 0, 0), glm::vec3(1, 0, 0))); // Bottom right
    vertices.push_back(VertexFormat(glm::vec3(30, 3, 0), glm::vec3(1, 0, 0))); // Top right
    vertices.push_back(VertexFormat(glm::vec3(0, 3, 0), glm::vec3(1, 0, 0)));  // Top left

    indices = { 0, 1, 2, 0, 2, 3 };

    healthBarRed = new Mesh("health_bar_red");
    healthBarRed->SetDrawMode(GL_TRIANGLES);
    healthBarRed->InitFromData(vertices, indices);

    vertices.clear();
    indices.clear();

    vertices.push_back(VertexFormat(glm::vec3(0, 0, 0), glm::vec3(1, 1, 0.2f)));  // Bottom left
    vertices.push_back(VertexFormat(glm::vec3(30, 0, 0), glm::vec3(1, 1, 0.2f))); // Bottom right
    vertices.push_back(VertexFormat(glm::vec3(30, 3, 0), glm::vec3(1, 1, 0.2f))); // Top right
    vertices.push_back(VertexFormat(glm::vec3(0, 3, 0), glm::vec3(1, 1, 0.2f)));  // Top left

    indices = { 0, 1, 2, 0, 2, 3 };

    healthBarYellow = new Mesh("health_bar_yellow");
    healthBarYellow->SetDrawMode(GL_TRIANGLES);
    healthBarYellow->InitFromData(vertices, indices);

    vertices.clear();
    indices.clear();

    vertices.push_back(VertexFormat(glm::vec3(0, 0, 0), glm::vec3(1.0, 0.55, 0.0)));  // Bottom left
    vertices.push_back(VertexFormat(glm::vec3(30, 0, 0), glm::vec3(1.0, 0.55, 0.0))); // Bottom right
    vertices.push_back(VertexFormat(glm::vec3(30, 3, 0), glm::vec3(1.0, 0.55, 0.0))); // Top right
    vertices.push_back(VertexFormat(glm::vec3(0, 3, 0), glm::vec3(1.0, 0.55, 0.0)));  // Top left

    indices = { 0, 1, 2, 0, 2, 3 };

    healthBarOrange = new Mesh("health_bar_orange");
    healthBarOrange->SetDrawMode(GL_TRIANGLES);
    healthBarOrange->InitFromData(vertices, indices);

    vertices.clear();
    indices.clear();

    vertices.push_back(VertexFormat(glm::vec3(0, 0, 0), glm::vec3(0.0f, 1.0f, 0.0f)));  // Bottom left
    vertices.push_back(VertexFormat(glm::vec3(30, 0, 0), glm::vec3(0.0f, 1.0f, 0.0f))); // Bottom right
    vertices.push_back(VertexFormat(glm::vec3(30, 3, 0), glm::vec3(0.0f, 1.0f, 0.0f))); // Top right
    vertices.push_back(VertexFormat(glm::vec3(0, 3, 0), glm::vec3(0.0f, 1.0f, 0.0f)));  // Top left

    indices = { 0, 1, 2, 0, 2, 3 };

    healthBarGreen = new Mesh("health_bar_green");
    healthBarGreen->SetDrawMode(GL_TRIANGLES);
    healthBarGreen->InitFromData(vertices, indices);

    vertices.clear();
    vertices.push_back(VertexFormat(glm::vec3(-frameThickness, -frameThickness, 0), glm::vec3(0, 0, 0)));  // Bottom left
    vertices.push_back(VertexFormat(glm::vec3(30 + frameThickness, -frameThickness, 0), glm::vec3(0, 0, 0))); // Bottom right
    vertices.push_back(VertexFormat(glm::vec3(30 + frameThickness, 3 + frameThickness, 0), glm::vec3(0, 0, 0))); // Top right
    vertices.push_back(VertexFormat(glm::vec3(-frameThickness, 3 + frameThickness, 0), glm::vec3(0, 0, 0)));  // Top left

    indices = { 0, 1, 2, 0, 2, 3 };

    // Create frame mesh
    healthBarFrame = new Mesh("health_bar_frame");
    healthBarFrame->SetDrawMode(GL_TRIANGLES);
    healthBarFrame->InitFromData(vertices, indices);
}

void Tema1::RenderHealthBar(const Tank& tank)
{
    float healthPercentage = std::max(0.0f, std::min(tank.health, 200.0f)) / 200.0f;

    glm::mat4 frameMatrix = glm::mat4(1);
    frameMatrix = glm::translate(frameMatrix, glm::vec3(tank.position.x - 10, tank.position.y + 25, 0));


    // Position the health bar above the tank
    glm::mat4 modelMatrix = glm::mat4(1);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(tank.position.x - 10, tank.position.y + 25, 0));

    glm::mat4 foregroundMatrix = glm::mat4(1);
    foregroundMatrix = glm::translate(foregroundMatrix, glm::vec3(tank.position.x - 10, tank.position.y + 25, 0));

    // Determine the color and scale based on hp
    if (healthPercentage > 0.75) {
        // Green for 76% to 100%
        foregroundMatrix = glm::scale(foregroundMatrix, glm::vec3(healthPercentage * 1.0f, 1.0f, 1.0f));
        RenderMesh(healthBarGreen, shaders["VertexColor"], foregroundMatrix);
    }
    else if (healthPercentage > 0.50) {
        // Yellow for 51% to 75%
        foregroundMatrix = glm::scale(foregroundMatrix, glm::vec3(healthPercentage * 1.0f, 1.0f, 1.0f));
        RenderMesh(healthBarYellow, shaders["VertexColor"], foregroundMatrix);
    }
    else if (healthPercentage > 0.25) {
        // Orange for 26% to 50%
        foregroundMatrix = glm::scale(foregroundMatrix, glm::vec3(healthPercentage * 1.0f, 1.0f, 1.0f));
        RenderMesh(healthBarOrange, shaders["VertexColor"], foregroundMatrix);
    }
    else if (healthPercentage > 0.0) {
        // Red for 1% to 25%
        foregroundMatrix = glm::scale(foregroundMatrix, glm::vec3(healthPercentage * 1.0f, 1.0f, 1.0f));
        RenderMesh(healthBarRed, shaders["VertexColor"], foregroundMatrix);
    }
    RenderMesh(healthBarFrame, shaders["VertexColor"], frameMatrix);
}

void Tema1::CreateParticleMesh() {
    vector<VertexFormat> vertices;
    vector<unsigned int> indices;

    // Create a simple triangle for the particle
    vertices.push_back(VertexFormat(glm::vec3(0, 2, 0), glm::vec3(1.0, 0.39, 0.28)));
    vertices.push_back(VertexFormat(glm::vec3(-1, 0, 0), glm::vec3(1.0, 0.39, 0.28)));
    vertices.push_back(VertexFormat(glm::vec3(1, 0, 0), glm::vec3(1.0, 0.39, 0.28)));

    indices = { 0, 1, 2 };

    particleMesh = new Mesh("particle");
    particleMesh->SetDrawMode(GL_TRIANGLES);
    particleMesh->InitFromData(vertices, indices);
}

void Tema1::CreateExplosion(const glm::vec2& position) {
    ExplosionEffect explosion;
    explosion.position = position;
    explosion.duration = PARTICLE_MAX_LIFETIME;
    explosion.active = true;

    // Create particles for the explosion
    for (int i = 0; i < PARTICLES_PER_EXPLOSION; i++) {
        Particle particle;
        particle.position = position;

        // Random angle for particle direction
        float angle = static_cast<float>(rand()) / RAND_MAX * 2 * M_PI;
        float speed = PARTICLE_MIN_SPEED + static_cast<float>(rand()) / RAND_MAX * (PARTICLE_MAX_SPEED - PARTICLE_MIN_SPEED);

        particle.velocity = glm::vec2(
            speed * cos(angle),
            speed * sin(angle)
        );

        // Random rotation and scale
        particle.rotation = static_cast<float>(rand()) / RAND_MAX * 2 * M_PI;
        particle.rotationSpeed = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 10.0f;
        particle.lifetime = PARTICLE_MAX_LIFETIME;
        particle.scale = 1.0f + static_cast<float>(rand()) / RAND_MAX * 1.5f;

        // Random colors for fire effect
        float r = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.2f;
        float g = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
        float b = 0.1f + static_cast<float>(rand()) / RAND_MAX * 0.2f;
        particle.color = glm::vec3(r, g, b);

        particle.active = true;
        explosion.particles.push_back(particle);
    }

    explosions.push_back(explosion);
}

void Tema1::CreateGunSmoke(const glm::vec2& position, float angle) {
    ExplosionEffect smoke;
    smoke.position = position;
    smoke.duration = SMOKE_MAX_LIFETIME;
    smoke.active = true;

    // Create particles for the smoke effect
    for (int i = 0; i < SMOKE_PARTICLES; i++) {
        Particle particle;
        particle.position = position;

        // Concentrate particles in the direction of the shot
        float spreadAngle = angle + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * M_PI / 3;  // 60-degree
        float speed = SMOKE_MIN_SPEED + static_cast<float>(rand()) / RAND_MAX * (SMOKE_MAX_SPEED - SMOKE_MIN_SPEED);

        particle.velocity = glm::vec2(
            speed * cos(spreadAngle),
            speed * sin(spreadAngle)
        );

        // Smaller rotation and scale for smoke
        particle.rotation = static_cast<float>(rand()) / RAND_MAX * 2 * M_PI;
        particle.rotationSpeed = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 5.0f;
        particle.lifetime = SMOKE_MAX_LIFETIME * (0.5f + static_cast<float>(rand()) / RAND_MAX * 0.5f);
        particle.scale = 0.3f + static_cast<float>(rand()) / RAND_MAX * 0.4f;  // Smaller particles

        // Smoke colors
        float brightness = 0.6f + static_cast<float>(rand()) / RAND_MAX * 0.2f;
        particle.color = glm::vec3(1, 1, 1);

        particle.active = true;
        smoke.particles.push_back(particle);
    }
    explosions.push_back(smoke);
}

void Tema1::UpdateExplosions(float deltaTimeSeconds) {
    for (auto& explosion : explosions) {
        if (!explosion.active) continue;

        explosion.duration -= deltaTimeSeconds;
        if (explosion.duration <= 0) {
            explosion.active = false;
            continue;
        }

        for (auto& particle : explosion.particles) {
            if (!particle.active) continue;

            // Update particle position
            particle.position += particle.velocity * deltaTimeSeconds;

            // Reduced gravity for smoke effect
            particle.velocity.y -= SMOKE_GRAVITY * deltaTimeSeconds;

            // Slow down particles over time for smoke effect
            particle.velocity *= (1.0f - deltaTimeSeconds * 0.5f);

            // Update rotation
            particle.rotation += particle.rotationSpeed * deltaTimeSeconds;

            // Gradually increase scale for smoke dissipation effect
            particle.scale += deltaTimeSeconds * 0.5f;

            // Update lifetime and fade
            particle.lifetime -= deltaTimeSeconds;
            if (particle.lifetime <= 0) {
                particle.active = false;
            }
        }
    }

    // Remove inactive explosions
    explosions.erase(
        std::remove_if(explosions.begin(), explosions.end(),
            [](const ExplosionEffect& e) { return !e.active; }),
        explosions.end()
    );
}

void Tema1::RenderExplosions() {
    for (const auto& explosion : explosions) {
        if (!explosion.active) continue;

        for (const auto& particle : explosion.particles) {
            if (!particle.active) continue;

            float alpha = particle.lifetime / PARTICLE_MAX_LIFETIME;
            glm::vec3 fadeColor = particle.color * alpha;

            glm::mat4 modelMatrix = glm::mat4(1);
            modelMatrix = glm::translate(modelMatrix, glm::vec3(particle.position, 0));
            modelMatrix = glm::rotate(modelMatrix, particle.rotation, glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, glm::vec3(particle.scale));

            // Create a custom shader matrix for color fade
            glm::mat4 colorMatrix = glm::mat4(1);
            colorMatrix[0][0] = fadeColor.r;
            colorMatrix[1][1] = fadeColor.g;
            colorMatrix[2][2] = fadeColor.b;
            colorMatrix[3][3] = alpha;

            RenderMesh(particleMesh, shaders["VertexColor"], modelMatrix * colorMatrix);
        }
    }
}

void Tema1::CreateCloudMesh()
{
    vector<VertexFormat> cloudVertices;
    vector<unsigned int> cloudIndices;

    // Cloud color - soft white
    glm::vec3 cloudColor(0.95f, 0.95f, 0.95f);

    // Create several circles that overlap to form a cloud shape
    const int segments = 32;
    float radiusMain = 10.0f;
    vector<glm::vec2> cloudParts = {
        glm::vec2(0, 0),      // Center circle
        glm::vec2(-8, 2),     // Left circle
        glm::vec2(8, 2),      // Right circle
        glm::vec2(-4, 4),     // Top left circle
        glm::vec2(4, 4),      // Top right circle
    };

    int baseIndex = 0;
    for (const auto& center : cloudParts) {
        // Add center vertex
        cloudVertices.push_back(VertexFormat(
            glm::vec3(center.x, center.y, 0),
            cloudColor
        ));

        // Add perimeter vertices
        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            float x = center.x + radiusMain * cos(angle);
            float y = center.y + radiusMain * sin(angle);
            cloudVertices.push_back(VertexFormat(
                glm::vec3(x, y, 0),
                cloudColor
            ));

            // Add triangles
            if (i < segments - 1) {
                cloudIndices.push_back(baseIndex);
                cloudIndices.push_back(baseIndex + i + 1);
                cloudIndices.push_back(baseIndex + i + 2);
            }
            else {
                cloudIndices.push_back(baseIndex);
                cloudIndices.push_back(baseIndex + segments);
                cloudIndices.push_back(baseIndex + 1);
            }
        }

        baseIndex += segments + 1;
    }

    cloudMesh = new Mesh("cloud");
    cloudMesh->SetDrawMode(GL_TRIANGLES);
    cloudMesh->InitFromData(cloudVertices, cloudIndices);
}

void Tema1::InitializeClouds()
{
    // Create several clouds with random positions and speeds
    for (int i = 0; i < 8; i++) {
        Cloud cloud{};
        cloud.position = glm::vec2(
            rand() % (int)TERRAIN_WIDTH,  // Random x position
            140 + rand() % 80             // Random height
        );
        cloud.scale = 0.7f + (rand() % 10) / 10.0f;  // Random scale
        cloud.speed = 5.0f + (rand() % 10) / 20.0f ;        // Random speed
        clouds.push_back(cloud);
    }
}

void Tema1::UpdateClouds(float deltaTime)
{
    for (auto& cloud : clouds) {
        // Move clouds from left to right
        cloud.position.x += cloud.speed * deltaTime;

        // If cloud moves off screen, reset to left side with new random height
        if (cloud.position.x > TERRAIN_WIDTH + 100) {
            cloud.position.x = -50;
            cloud.position.y = 150 + rand() % 50;
            cloud.scale = 0.7f + (rand() % 10) / 10.0f;
            cloud.speed = 1.0f + (rand() % 10);
        }
    }
}
