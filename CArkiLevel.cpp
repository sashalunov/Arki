#include "stdafx.h"
#include "CArkiLevel.h"

int scorePalette[] = {
    10,   // Red
    50,   // Gold
    100,   // Yellow
    150,   // Green
    250,   // Cyan
    400,  // Blue
    1010,  // Pink
    5000   // White
};
// Classic Arkanoid Palette (Red, Gold, Yellow, Green, Cyan, Blue, Pink, White)
D3DCOLOR palette[] = {
    D3DCOLOR_XRGB(255, 50, 50),   // Row 0: Red
    D3DCOLOR_XRGB(255, 180, 0),   // Row 1: Gold
    D3DCOLOR_XRGB(255, 255, 0),   // Row 2: Yellow
    D3DCOLOR_XRGB(50, 255, 50),   // Row 3: Green
    D3DCOLOR_XRGB(0, 255, 255),   // Row 4: Cyan
    D3DCOLOR_XRGB(50, 50, 255),   // Row 5: Blue
    D3DCOLOR_XRGB(255, 50, 255),  // Row 6: Pink
    D3DCOLOR_XRGB(200, 200, 200)  // Row 7: White
};
int paletteSize = 8;

void CArkiLevel::GenerateRandomLevel(int rows, int cols, bool symmetric = true)
{
    // 1. Clear existing blocks
    Clear();

    // 2. Configuration (Adjust these to match your block model size)
    // Assuming Block HalfSize is (1.0, 0.5, 0.5) -> Full Size is (2.0, 1.0, 1.0)
    float blockWidth = 2.0f;
    float blockHeight = 1.0f;
    float padding = 0.1f; // Small gap between bricks looks better
    // Vertical Start Position (Where the top-most row begins)
    float startY = 18.0f;
    // 3. Calculate "Start X" to ensure the whole grid is centered on 0
    float totalWidth = cols * (blockWidth + padding);
    float startX = -(totalWidth / 2.0f) + (blockWidth / 2.0f); // Offset by half-block to center anchors

    // 4. Random Seed (Call srand(time(0)) in main if you haven't!)

    if (symmetric) {
        for (int r = 0; r < rows; r++)
        {
            // Iterate only half the columns (rounded up for odd middle column)
            int mid = (cols + 1) / 2;
            // Pick color based on Row Index (cycle if we have more rows than colors)
            D3DCOLOR rowColor = palette[r % paletteSize];
            int rowScore = scorePalette[r % paletteSize];
            for (int c = 0; c < mid; c++)
            {
                // 75% chance to place a brick here
                if ((rand() % 100) < 75)
                {
                    // 1. Place Left Block
                    float x1 = startX + c * (blockWidth + padding);
                    float y = startY - r * (blockHeight + padding);

                    D3DXVECTOR3 halfSize(1.0f, 0.5f, 0.5f);
                    m_blocks.push_back(new CArkiBlock(m_pDynamicsWorld, D3DXVECTOR3(x1, y, 0), halfSize, rowColor, rowScore));

                    // 2. Place Mirrored Right Block (if not the exact center)
                    int mirrorCol = (cols - 1) - c;
                    if (mirrorCol != c)
                    {
                        float x2 = startX + mirrorCol * (blockWidth + padding);
                        m_blocks.push_back(new CArkiBlock(m_pDynamicsWorld, D3DXVECTOR3(x2, y, 0), halfSize, rowColor, rowScore));
                    }
                }
            }
        }
    }
    else
    {
        // 5. Generation Loop
        for (int r = 0; r < rows; r++)
        {
            // Pick color based on Row Index (cycle if we have more rows than colors)
            D3DCOLOR rowColor = palette[r % paletteSize];
            int rowScore = scorePalette[r % paletteSize];

            for (int c = 0; c < cols; c++)
            {
                bool createBlock = false;

                // Pure random (Chaos mode)
                createBlock = (rand() % 100) < 70;

                if (createBlock)
                {
                    // Calculate Position
                    float x = startX + c * (blockWidth + padding);
                    float y = startY - r * (blockHeight + padding); // Go down as rows increase
                    float z = 0.0f;

                    D3DXVECTOR3 pos(x, y, z);
                    D3DXVECTOR3 halfSize(blockWidth * 0.5f, blockHeight * 0.5f, 0.5f);

                    CArkiBlock* b = new CArkiBlock(m_pDynamicsWorld, pos, halfSize, rowColor, rowScore);
                    m_blocks.push_back(b);
                }
            }
        }

    }
}

// Add to CArkiLevel.cpp
#include <cmath> // Required for sqrt (distance calculation)

void CArkiLevel::GenerateIslandLevel(int rows, int cols, int numIslands)
{
    // 1. Clear existing
    Clear();

    // 2. Setup Configuration (Matches your existing sizing)
    float blockWidth = 2.0f;
    float blockHeight = 1.0f;
    float padding = 0.1f;
    float startY = 18.0f;
    float totalWidth = cols * (blockWidth + padding);
    float startX = -(totalWidth / 2.0f) + (blockWidth / 2.0f);

    // Temp grid to store island IDs (0 = empty)
    std::vector<std::vector<int>> grid(rows, std::vector<int>(cols, 0));

    // Structure to hold our "Island" centers
    struct IslandSeed {
        int r, c;      // Grid position
        int id;        // Unique ID
        float radius;  // How big this island is
        D3DCOLOR color;
    };

    std::vector<IslandSeed> seeds;
    int mid = (cols + 1) / 2; // We only generate for the left half, then mirror

    // 3. Create Random Seeds (Centers of islands)
    for (int i = 0; i < numIslands; i++)
    {
        IslandSeed s;
        // Keep islands away from the very bottom rows to give player space
        s.r = rand() % (rows - 2);
        // Keep within left half
        s.c = rand() % mid ;
        s.id = i + 1;
        // Random size: small (1.5) to large (3.5)
        s.radius = 0.1f + (rand() % 15) / 10.0f;

        // Pick a random color from your global palette
        s.color = palette[rand() % paletteSize];

        seeds.push_back(s);
    }

    // 4. "Grow" the islands based on distance
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < mid; c++)
        {
            for (const auto& s : seeds)
            {
                // Calculate distance from this block (r,c) to the seed (s.r, s.c)
                float dr = (float)(r - s.r);
                float dc = (float)(c - s.c);
                float dist = sqrt(dr * dr + dc * dc);

                // If within radius, this block becomes part of the island
                // We add a tiny bit of noise ((rand()%10)/20.0f) so it's not a perfect circle
                if (dist <= s.radius + ((rand() % 10) / 20.0f))
                {
                    grid[r][c] = s.id;
                }
            }
        }
    }

    // 5. Instantiate the Blocks (with Mirroring)
    D3DXVECTOR3 halfSize(1.0f, 0.5f, 0.5f);

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < mid; c++)
        {
            int islandId = grid[r][c];

            // If this cell belongs to an island
            if (islandId > 0)
            {
                // Find the color associated with this island ID
                D3DCOLOR col = D3DCOLOR_XRGB(255, 255, 255);
                for (const auto& s : seeds) {
                    if (s.id == islandId) col = s.color;
                }

                // --- Place Left Block ---
                float x1 = startX + c * (blockWidth + padding);
                float y = startY - r * (blockHeight + padding);
                m_blocks.push_back(new CArkiBlock(m_pDynamicsWorld, D3DXVECTOR3(x1, y, 0), halfSize, col, 0));

                // --- Place Mirrored Right Block ---
                int mirrorCol = (cols - 1) - c;
                // Don't double-place the center column
                if (mirrorCol != c)
                {
                    float x2 = startX + mirrorCol * (blockWidth + padding);
                    m_blocks.push_back(new CArkiBlock(m_pDynamicsWorld, D3DXVECTOR3(x2, y, 0), halfSize, col,0));
                }
            }
        }
    }
}


void CArkiLevel::GenerateMathLevel(int rows, int cols)
{
    Clear();

    // 1. Setup Configuration
    float blockWidth = 2.0f;
    float blockHeight = 1.0f;
    float padding = 0.1f;
    float startY = 18.0f;
    float totalWidth = cols * (blockWidth + padding);
    float startX = -(totalWidth / 2.0f) + (blockWidth / 2.0f);

    // 2. Pick a random "Formula" for this level
    // 0 = Concentric Rings, 1 = Interference Waves, 2 = Plasmas
    int patternType = rand() % 3;

    // Scale factors zoom the math pattern in/out
    float scaleX = (rand() % 5 + 3) / 10.0f; // Random 0.3 to 0.8
    float scaleY = (rand() % 5 + 3) / 10.0f;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            float val = 0.0f;

            // Normalize coordinates (-1.0 to 1.0) for math flexibility
            float nx = (c - cols / 2.0f) * scaleX;
            float ny = (r - rows / 2.0f) * scaleY;

            // 3. Apply the Math
            switch (patternType)
            {
            case 0: // Concentric Rings / Target
            {
                float dist = sqrt(nx * nx + ny * ny);
                val = sin(dist * 5.0f); // 5.0f controls ring density
            }
            break;
            case 1: // Interference Waves (Waffle pattern)
                val = sin(nx * 4.0f) + cos(ny * 4.0f);
                break;
            case 2: // "Plasma" diagonal ripples
                val = sin(nx * 3.0f + ny * 3.0f) * cos(nx * 3.0f - ny * 3.0f);
                break;
            }

            // 4. Thresholding
            // If the math value is high enough, place a block.
            // Adjusting '0.2f' changes how "thick" the structures are.
            if (val > 0.2f)
            {
                // 5. Coloring Strategy
                // Instead of row-based color, let's color based on the Math Value
                // This creates nice gradients that match the shape.
                int colorIndex = (int)(fabs(val) * 8) % paletteSize;
                D3DCOLOR blockColor = palette[colorIndex];
                int rowScore = scorePalette[colorIndex];

                // Calculate World Position
                float x = startX + c * (blockWidth + padding);
                float y = startY - r * (blockHeight + padding);

                D3DXVECTOR3 halfSize(1.0f, 0.5f, 0.5f);
                m_blocks.push_back(new CArkiBlock(m_pDynamicsWorld, D3DXVECTOR3(x, y, 0), halfSize, blockColor, rowScore));
            }
        }
    }
}

// Add to CArkiLevel.cpp

void CArkiLevel::GenerateFractalLevel(int rows, int cols)
{
    Clear();

    // 1. Setup Grid
    float blockWidth = 2.0f;
    float blockHeight = 1.0f;
    float padding = 0.1f;
    float startY = 18.0f;
    float totalWidth = cols * (blockWidth + padding);
    float startX = -(totalWidth / 2.0f) + (blockWidth / 2.0f);

    // 2. Pick a Fractal Variation
    // 0 = Julia Set (Swirls/Islands)
    // 1 = Sierpinski (Triangles)
    // 2 = XOR Pattern (Sci-fi Grid)
    int fractalType = rand() % 3;

    // 3. Randomize Parameters (Zoom/Offset) so no two levels are identical
    // Zoom: Smaller number = bigger shapes
    float zoom = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    // Parameters for Julia Set
    float cRe = -0.7f, cIm = 0.27015f; // Default "Seahorse" valley

    if (fractalType == 0) // Randomize Julia Set
    {
        zoom = 0.8f + (rand() % 10) / 10.0f; // 0.8 to 1.8
        // Randomize the constant 'C' to change the island shapes entirely
        cRe = -0.8f + (rand() % 100) / 100.0f; // -0.8 to 0.2
        cIm = 0.2f + (rand() % 50) / 100.0f;   // 0.2 to 0.7
    }
    else if (fractalType == 1) // Sierpinski
    {
        // Random bit shift creates different triangle densities
        zoom = (rand() % 2 == 0) ? 1.0f : 2.0f;
    }

    // 4. Generation Loop
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int colorIndex = -1; // -1 means "Don't place block"

            if (fractalType == 0)
            {
                // --- VARIATION A: JULIA SET (Swirly Islands) ---
                // Map grid to Complex Plane (-1.5 to 1.5)
                float newRe = 1.5f * (c - cols / 2.0f) / (0.5f * zoom * cols) + offsetX;
                float newIm = (r - rows / 2.0f) / (0.5f * zoom * rows) + offsetY;

                // Iterate the formula: Z = Z^2 + C
                int i;
                int maxIterations = 16; // Higher = more detail, fewer blocks
                for (i = 0; i < maxIterations; i++)
                {
                    float oldRe = newRe;
                    float oldIm = newIm;

                    newRe = oldRe * oldRe - oldIm * oldIm + cRe;
                    newIm = 2 * oldRe * oldIm + cIm;

                    if ((newRe * newRe + newIm * newIm) > 4) break; // Escaped
                }

                // Use the "Escape Time" to determine color
                // If i == maxIterations, it's inside the set (usually black/empty)
                // If i is small, it's on the edge (colorful)
                if (i < maxIterations && i > 3)
                    colorIndex = i % paletteSize;
            }
            else if (fractalType == 1)
            {
                // --- VARIATION B: SIERPINSKI TRIANGLE (Geometry) ---
                // Simple bitwise AND logic creates triangles
                int x = (int)(c * zoom);
                int y = (int)(r * zoom); // Flip r to orient triangle correctly if needed

                // The classic formula: (x & y) == 0
                if ((x & y) == 0)
                    colorIndex = (x + y) % paletteSize;
            }
            else
            {
                // --- VARIATION C: XOR TEXTURE (Sci-fi) ---
                int x = c;
                int y = r;
                // XOR patterns create "alien data" looks
                int val = ((x ^ y) % 5);

                if (val < 2) // Only spawn on certain values
                    colorIndex = (x * y) % paletteSize;
            }

            // 5. Place Block if valid
            if (colorIndex >= 0)
            {
                D3DCOLOR col = palette[colorIndex];

                // Calculate World Position
                float xPos = startX + c * (blockWidth + padding);
                float yPos = startY - r * (blockHeight + padding);

                D3DXVECTOR3 halfSize(1.0f, 0.5f, 0.5f);
                int rowScore = scorePalette[colorIndex];

                // Create Block
                CArkiBlock* b = new CArkiBlock(m_pDynamicsWorld, D3DXVECTOR3(xPos, yPos, 0), halfSize, col, rowScore);

                // Apply Score (Assuming you have the scorePalette from previous step)
                // If you haven't added 'scorePalette' yet, you can use: b->scoreValue = (colorIndex + 1) * 10;
                //b->scoreValue = (colorIndex + 1) * 10 + 50;
				//b->scoreValue = scorePalette[colorIndex % paletteSize];

                m_blocks.push_back(b);
            }
        }
    }
}

void CArkiLevel::GenerateMengerLevel(int rows, int cols)
{
    Clear();

    float blockWidth = 2.0f;
    float blockHeight = 1.0f;
    float padding = 0.1f;
    float startY = 18.0f;
    float totalWidth = cols * (blockWidth + padding);
    float startX = -(totalWidth / 2.0f) + (blockWidth / 2.0f);

    // --- 1. Randomize the "Viewport" ---
    // The Menger sponge is infinite. By picking a random starting X/Y,
    // we generate a completely different slice of the fractal every time.
    int offsetX = rand() % 10000;
    int offsetY = rand() % 10000;

    // --- 2. Randomize "Decay" ---
    // A perfect fractal can be boring. Let's add a "Ruins" factor.
    // 10% to 30% chance to randomly break a block or fill a hole.
    int erosionRate = 10 + (rand() % 20);

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            // Map grid position to the random fractal coordinates
            int x = c + offsetX;
            int y = r + offsetY;

            bool isHole = false;
            int tempX = x;
            int tempY = y;

            // Standard Menger Logic (Base 3)
            while (tempX > 0 || tempY > 0)
            {
                // If the middle digit of both coordinates is 1, it's a hole.
                if ((tempX % 3 == 1) && (tempY % 3 == 1))
                {
                    isHole = true;
                    break;
                }
                tempX /= 3;
                tempY /= 3;
            }

            // --- 3. Apply Random Chaos (The "Fix") ---
            // Even if math says "Hole", sometimes place a block (debris)
            // Even if math says "Block", sometimes destroy it (erosion)
            bool randomEvent = (rand() % 100) < erosionRate;

            if (randomEvent) {
                // Flip the state (Block becomes Hole, Hole becomes Block)
                isHole = !isHole;
            }

            // Logic: Place block if it's NOT a hole
            if (!isHole)
            {
                // Color calculation: use the fractal coordinates so the pattern matches the color
                // We use bitwise operators on x/y to make the colors look "techy"
                int colorIndex = (x ^ y) % paletteSize;
                D3DCOLOR col = palette[colorIndex];

                // Score: Deeper/Complex patterns get higher scores
                int score = 50 + (colorIndex * 15);

                float xPos = startX + c * (blockWidth + padding);
                float yPos = startY - r * (blockHeight + padding);

                D3DXVECTOR3 halfSize(1.0f, 0.5f, 0.5f);
                m_blocks.push_back(new CArkiBlock(m_pDynamicsWorld, D3DXVECTOR3(xPos, yPos, 0), halfSize, col, score));
            }
        }
    }
}

void CArkiLevel::GenerateCaveLevel(int rows, int cols)
{
    Clear();

    float blockWidth = 2.0f;
    float blockHeight = 1.0f;
    float padding = 0.1f;
    float startY = 18.0f;
    float totalWidth = cols * (blockWidth + padding);
    float startX = -(totalWidth / 2.0f) + (blockWidth / 2.0f);

    // 1. Create a temporary integer map (0 or 1)
    std::vector<std::vector<int>> map(rows, std::vector<int>(cols, 0));

    // 2. Random Initialization (Fill ~45% of the map)
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if ((rand() % 100) < 45) map[r][c] = 1;
        }
    }

    // 3. Run "Smoothing" steps (Automata rules)
    // Running this 2-4 times smoothes the noise into caves.
    int iterations = 4;
    for (int i = 0; i < iterations; i++)
    {
        std::vector<std::vector<int>> nextMap = map;
        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < cols; c++)
            {
                // Count neighbors
                int neighbors = 0;
                for (int nr = r - 1; nr <= r + 1; nr++) {
                    for (int nc = c - 1; nc <= c + 1; nc++) {
                        if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                            if (nr != r || nc != c) // Don't count self
                                neighbors += map[nr][nc];
                        }
                        else {
                            neighbors++; // Borders act as walls (optional)
                        }
                    }
                }

                // The Rules:
                // If I'm alive and have > 3 neighbors, stay alive.
                // If I'm dead and have > 4 neighbors, become alive.
                if (map[r][c] == 1) {
                    if (neighbors < 3) nextMap[r][c] = 0; // Starvation
                    else nextMap[r][c] = 1;
                }
                else {
                    if (neighbors > 4) nextMap[r][c] = 1; // Birth
                }
            }
        }
        map = nextMap;
    }

    // 4. Generate Blocks from final map
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            if (map[r][c] == 1)
            {
                // Color strategy: Gradient from top to bottom
                int colorIndex = (r * paletteSize / rows) % paletteSize;
                D3DCOLOR col = palette[colorIndex];
                int score = 100;

                float xPos = startX + c * (blockWidth + padding);
                float yPos = startY - r * (blockHeight + padding);
                D3DXVECTOR3 halfSize(1.0f, 0.5f, 0.5f);

                m_blocks.push_back(new CArkiBlock(m_pDynamicsWorld, D3DXVECTOR3(xPos, yPos, 0), halfSize, col, score));
            }
        }
    }
}

