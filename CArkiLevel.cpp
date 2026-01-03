#include "stdafx.h"
#include "Logger.h"
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

// -----------------------------------------------------------------------
// SAVE: Serializes struct to Human-Readable JSON text
// -----------------------------------------------------------------------
void CArkiLevel::SaveLevel(const std::wstring& filename)
{
    // 1. Convert C++ Struct to JSON Object
    // This uses the macro we defined in the header automatically.
    json j = m_currentParams;

    // 2. Open File Stream
    std::ofstream outFile(filename.c_str());

    if (outFile.is_open()) {
        // 3. Write to file
        // setw(4) enables "Pretty Printing" (indentation) so it looks nice in Notepad
        outFile << j.dump(4);
        outFile.close();
    }
}

// -----------------------------------------------------------------------
// LOAD: Parses JSON text back into C++ Struct
// -----------------------------------------------------------------------
void CArkiLevel::LoadLevel(const std::wstring& filename)
{
    std::ifstream inFile(filename.c_str());

    if (!inFile.is_open()) {
        // Optional: Log error
        _log(L"Failed to open save file: %s", filename.c_str());
        return;
    }

    try {
        // 1. Parse the file into a JSON object
        json j;
        inFile >> j;

        // 2. Convert JSON object back to LevelParams struct
        LevelParams loadedParams = j.get<LevelParams>();

        // 3. Regenerate the level
        GenerateMathLevel(loadedParams);
    }
    catch (json::parse_error& e) {
        // Handles corrupted files gracefully
        _log(L"JSON Parse Error: %s" , e.what() );
    }
    catch (json::type_error& e) {
        // Handles cases where a number is a string, etc.
        _log(L"JSON Type Error: %s", e.what() );
    }

    inFile.close();
}

void CArkiLevel::UpdateParameters(const LevelParams& params)
{
    m_currentParams = params;

    srand(params.seed);

    m_currentParams.offsetX = rand() % 1000;
    m_currentParams.offsetY = rand() % 1000;
    m_currentParams.scaleX = (rand() % 5 + 3) / 10.0f;
    m_currentParams.scaleY = (rand() % 5 + 3) / 10.0f;

    // Auto-fix scales for specific fractals
    if (params.formulaType ==  FORMULA_SIERPINSKI_CARPET) {
        m_currentParams.scaleX = 1.0f; m_currentParams.scaleY = 1.0f;
    }
    if (params.formulaType == FORMULA_MANDELBROT) {
        m_currentParams.scaleX = 3.0f / m_currentParams.cols; m_currentParams.scaleY = 2.5f / m_currentParams.rows;
    }
    if (params.formulaType == FORMULA_JULIA || params.formulaType == FORMULA_SIERPINSKI) {
        m_currentParams.offsetX = 0; m_currentParams.offsetY = 0;
    }

}

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
    int patternType = 2;// rand() % 3;

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


void CArkiLevel::GenerateMathLevel(int rows, int cols, int patternType)
{
    Clear(); // Ensure old blocks are deleted

    // 1. Setup Configuration
    float blockWidth = 2.0f;
    float blockHeight = 1.0f;
    float padding = 0.1f;
    float startY = 18.0f;
    float totalWidth = cols * (blockWidth + padding);
    float startX = -(totalWidth / 2.0f) + (blockWidth / 2.0f);

    // Safety: Ensure we have a valid palette
    if (paletteSize <= 0) return;

    // 2. Setup Scaling (Zoom)
    // Default: Random variation for organic patterns
    float scaleX = (rand() % 5 + 3) / 10.0f; // 0.3 to 0.8
    float scaleY = (rand() % 5 + 3) / 10.0f;

    // OVERRIDE: Fractals need specific zoom levels to look correct
    if (patternType == 3) { scaleX = 1.0f; scaleY = 1.0f; } // Sierpinski needs integer mapping
    if (patternType == 4) { scaleX = 1.0f; scaleY = 1.0f; } // Sierpinski needs integer mapping
    if (patternType == 6) { scaleX = 3.0f / cols; scaleY = 2.5f / rows; } // Fit Mandelbrot to screen

    int randOffsetX = rand() % 100;
    int randOffsetY = rand() % 100;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            float val = 0.0f;

            // Normalized coordinates (-1.0 to 1.0 is standard, usually)
            float nx = (c - cols / 2.0f) * scaleX;
            float ny = (r - rows / 2.0f) * scaleY;

            // 3. Apply the Math
            switch (patternType)
            {
            case 0: // Concentric Rings
            {
                float dist = sqrt(nx * nx + ny * ny);
                val = sin(dist * 5.0f);
                break;
            }
            case 1: // Interference Waves
                val = sin(nx * 4.0f) + cos(ny * 4.0f);
                break;
            case 2: // Plasma
                val = sin(nx * 3.0f + ny * 3.0f) * cos(nx * 3.0f - ny * 3.0f);
                break;

                // --- NEW FORMULAS ---

            case 3: // Sierpinski Triangle (Bitwise Fractal)
            {
                
                    // 1. Convert to positive integers + Add Random Offset
                    // We use (rows - r) to flip it so the triangle base is at the bottom.
                    int xInt = (c * 2) + randOffsetX;       // *2 to zoom out slightly
                    int yInt = ((rows - r) * 2) + randOffsetY;

                    // 2. The Logic: (x & y) == 0 implies a solid block in Sierpinski
                    if ((xInt & yInt) == 0)
                    {
                        // 3. COLORING MAGIC
                        // Instead of just 1.0f, we create a value based on hierarchy.
                        // (xInt | yInt) gives us a value related to the structural layer.
                        // We normalize it to generate a color index later.

                        int structuralValue = (xInt | yInt);

                        // Creates a gradient that follows the triangle structures
                        val = 0.2f + (structuralValue % 10) / 10.0f;

                        // Boost it to ensure it passes the threshold
                        if (val < 0.2f) val = 0.25f;
                    }
                    else
                    {
                        val = 0.0f; // Hole
                    }
                    break;
            }

            case 4: // FORMULA_SIERPINSKI_CARPET (Denser & Less Voids)
            {
                // 1. Setup Integer Coordinates
                int tx = c + randOffsetX;
                int ty = (rows - r) + randOffsetY;

                bool isHole = false;
                int depth = 0;
                int maxDepth = 2; // <--- CRITICAL: Low number = Less Voids (Denser)
                // Set to 5 for a true empty fractal, 2 for a wall-like structure.

                // 2. Base-3 Carpet Logic
                while (tx > 0 || ty > 0)
                {
                    // If center square of a 3x3 grid, it's a hole
                    if (tx % 3 == 1 && ty % 3 == 1) {
                        isHole = true;
                        break;
                    }
                    tx /= 3;
                    ty /= 3;

                    // 3. Density Check
                    depth++;
                    if (depth >= maxDepth) break; // Stop checking tiny holes to keep it solid
                }

                if (!isHole) {
                    // Create a colorful banding effect based on position
                    val = 0.5f + (float)((c + r) % 10) / 20.0f;
                }
                else {
                    val = 0.0f;
                }
                break;
            }

            case 5: // Liquid Checkerboard
            {
                // Distortion factor
                float distort = sin(nx * 2.0f) * 0.5f;
                // Checkboard formula: sin(x) * sin(y) > 0
                val = sin((nx + distort) * 6.0f) * cos(ny * 6.0f);
                break;
            }

            case 6: // Mandelbrot Set (Iterative Fractal)
            {
                // Shift coordinates to center the fractal
                float mx = ((c - cols / 1.5f) * scaleX);
                float my = ((r - rows / 2.0f) * scaleY);

                float zx = 0.0f, zy = 0.0f;
                int maxIter = 16; // Low iterations for blocky style
                int iter = 0;

                while ((zx * zx + zy * zy < 4.0f) && (iter < maxIter)) {
                    float temp = zx * zx - zy * zy + mx;
                    zy = 2.0f * zx * zy + my;
                    zx = temp;
                    iter++;
                }

                // If iter == maxIter, it's inside the set (black usually).
                // We invert it to make the "Edge" interesting.
                if (iter < maxIter) {
                    // Map iteration count to value for coloring
                    val = (float)iter / (float)maxIter;
                }
                else {
                    val = 0.0f; // Inside the "lake" (empty)
                }
                break;
            }
            } // End Switch

            // 4. Thresholding & Placement
            // Note: Lowered threshold slightly to catch thin fractal lines
            if (val > 0.15f)
            {
                // 5. Coloring Strategy
                // Safe Indexing Logic
                float absVal = fabs(val);
                if (absVal > 100.0f) absVal = 100.0f; // Clamp to prevent overflow

                int colorIndex = (int)(absVal * 10) % paletteSize; // Increased multiplier for variety

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

// -----------------------------------------------------------------------
// GENERATE: Now uses the "Recipe" (Params) instead of randomizing locally
// -----------------------------------------------------------------------
void CArkiLevel::GenerateMathLevel(const LevelParams& params)
{
    // 1. Create a Grid Map to track which cells are filled
    // We use a flat vector: index = r * cols + c
    std::vector<bool> occupied(m_currentParams.rows * m_currentParams.cols, false);

    Clear();

    // 3. Setup Grid
    float blockWidth = 2.0f;
    float blockHeight = 1.0f;
    float padding = 0.1f;
    float startY = 18.0f;
    float totalWidth = m_currentParams.cols * (blockWidth + padding);
    float startX = -(totalWidth / 2.0f) + (blockWidth / 2.0f);

    // 4. Use Params for logic
    //int attempts = 0;
    // OVERRIDE: Fractals need specific zoom levels to look correct
   // if (params.formulaType == 3) { m_currentParams.scaleX = 1.0f; m_currentParams.scaleY = 1.0f; } // Sierpinski needs integer mapping
    //if (params.formulaType == 4) { m_currentParams.scaleX = 1.0f; m_currentParams.scaleY = 1.0f; } // Sierpinski needs integer mapping
    //if (params.formulaType == 6) { m_currentParams.scaleX = 3.0f / m_currentParams.cols; m_currentParams.scaleY = 2.5f / m_currentParams.rows; } // Fit Mandelbrot to screen

    // Note: The do-while loop for emptiness check needs care. 
    // Since we are forcing a Seed, rand() will result in the same outcome.
    // If a specific seed generates an empty level, we simply assume it's empty.
    // To fix emptiness, we would need to mutate the seed, but that changes the "saved" level.
    // For now, we run generation once.
        // Parameters for Julia Set
    float cRe = -0.725f, cIm = 0.27015f; // Default "Seahorse" valley
    float zoom = 1.0f;
    //zoom = 0.8f + (rand() % 10) / 10.0f; // 0.8 to 1.8
    // Randomize the constant 'C' to change the island shapes entirely
   // cRe = -0.8f + (rand() % 100) / 100.0f; // -0.8 to 0.2
    //cIm = 0.4f + (rand() % 80) / 100.0f;   // 0.2 to 0.7


    for (int r = 0; r < m_currentParams.rows; r++)
    {
        for (int c = 0; c < m_currentParams.cols; c++)
        {
            if (occupied[r * m_currentParams.cols + c]) continue;

            float val = 0.0f;
            float nx = (c - m_currentParams.cols / 2.0f) * m_currentParams.scaleX;
            float ny = (r - m_currentParams.rows / 2.0f) * m_currentParams.scaleY;

            // Use m_currentParams.offsetX/Y for Fractal cases
            int tx = c + m_currentParams.offsetX;
            int ty = (m_currentParams.rows - r) + m_currentParams.offsetY;

            bool isHole = false;
            int tempX = tx;
            int tempY = ty;
            int depth = 0;

            switch (m_currentParams.formulaType)
            {
            case FORMULA_RINGS:
                val = sin(sqrt(nx * nx + ny * ny) * 5.0f);
                break;
            case FORMULA_WAVES: // Interference Waves
                val = sin(nx * 4.0f) + cos(ny * 4.0f);
                break;
            case FORMULA_PLASMA: // Plasma
                val = sin(nx * 3.0f + ny * 3.0f) * cos(nx * 3.0f - ny * 3.0f);
                break;
            case FORMULA_SIERPINSKI: // Sierpinski Triangle (Bitwise Fractal)
            {
                // 1. Convert to positive integers + Add Random Offset
                // We use (rows - r) to flip it so the triangle base is at the bottom.
                int xInt = (c * 2) + m_currentParams.offsetX;       // *2 to zoom out slightly
                int yInt = ((m_currentParams.rows - r) * 2) + m_currentParams.offsetY;

                // 2. The Logic: (x & y) == 0 implies a solid block in Sierpinski
                if ((xInt & yInt) == 0)
                {
                    // 3. COLORING MAGIC
                    // Instead of just 1.0f, we create a value based on hierarchy.
                    // (xInt | yInt) gives us a value related to the structural layer.
                    // We normalize it to generate a color index later.

                    int structuralValue = (xInt | yInt);

                    // Creates a gradient that follows the triangle structures
                    val = 0.2f + (structuralValue % 10) / 10.0f;

                    // Boost it to ensure it passes the threshold
                    if (val < 0.2f) val = 0.25f;
                }
                else
                {
                    val = 0.0f; // Hole
                }
                break;
            }
            case FORMULA_SIERPINSKI_CARPET:
                // Use the deterministic offsets from params
                while (tempX > 0 || tempY > 0) {
                    if (tempX % 3 == 1 && tempY % 3 == 1) { isHole = true; break; }
                    tempX /= 3; tempY /= 3;
                    if (++depth > 2) break;
                }
                val = isHole ? 0.0f : 0.5f + (float)((c + r) % 10) / 20.0f;
                
                break;

            case FORMULA_LIQUID: // Liquid Checkerboard
            {
                // Distortion factor
                float distort = sin(nx * 2.0f) * 0.5f;
                // Checkboard formula: sin(x) * sin(y) > 0
                val = sin((nx + distort) * 6.0f) * cos(ny * 6.0f);
                break;
            }
            case FORMULA_JULIA:
            {
                // --- VARIATION A: JULIA SET (Swirly Islands) ---
                // Map grid to Complex Plane (-1.5 to 1.5)
                float newRe = 1.5f * (c - m_currentParams.cols / 2.0f) / (0.5f * zoom * m_currentParams.cols) + m_currentParams.offsetX;
                float newIm = (r - m_currentParams.rows / 2.0f) / (0.5f * zoom * m_currentParams.rows) + m_currentParams.offsetY;

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
                    val = (float)i / (float)maxIterations;
                else
					val = 0.0f; // Inside the set

				break;
            }
            case FORMULA_MANDELBROT: // Mandelbrot Set (Iterative Fractal)
            {
                // Shift coordinates to center the fractal
                float mx = ((c - m_currentParams.cols / 1.5f) * m_currentParams.scaleX);
                float my = ((r - m_currentParams.rows / 2.0f) * m_currentParams.scaleY);

                float zx = 0.0f, zy = 0.0f;
                int maxIter = 16; // Low iterations for blocky style
                int iter = 0;

                while ((zx * zx + zy * zy < 4.0f) && (iter < maxIter)) {
                    float temp = zx * zx - zy * zy + mx;
                    zy = 2.0f * zx * zy + my;
                    zx = temp;
                    iter++;
                }

                // If iter == maxIter, it's inside the set (black usually).
                // We invert it to make the "Edge" interesting.
                if (iter < maxIter) {
                    // Map iteration count to value for coloring
                    val = (float)iter / (float)maxIter;
                }
                else {
                    val = 0.0f; // Inside the "lake" (empty)
                }
                break;
            }
            default:
                val = 0.5f; // Fallback
                break;
            }

            // Threshold and Add Block
            if (val > 0.15f)
            {
                // 5. Coloring Strategy
                // Safe Indexing Logic
                float absVal = fabs(val);
                if (absVal > 100.0f) absVal = 100.0f; // Clamp to prevent overflow

                int colorIndex = (int)(absVal * 10) % paletteSize; // Increased multiplier for variety

                D3DCOLOR blockColor = palette[colorIndex];
                int rowScore = scorePalette[colorIndex];

                // Calculate World Position
                float x = startX + c * (blockWidth + padding);
                float y = startY - r * (blockHeight + padding);

                D3DXVECTOR3 halfSize(1.0f, 0.5f, 0.5f);
                m_blocks.push_back(new CArkiBlock(m_pDynamicsWorld, D3DXVECTOR3(x, y, 0), halfSize, blockColor, rowScore));
                occupied[r * m_currentParams.cols + c] = true;
            }
        }
    }
}





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
    int fractalType = 0;// rand() % 3;

    // 3. Randomize Parameters (Zoom/Offset) so no two levels are identical
    // Zoom: Smaller number = bigger shapes
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    // Parameters for Julia Set
    float cRe = -0.725f, cIm = 0.27015f; // Default "Seahorse" valley
    float zoom = 1.0f;

    if (fractalType == 0) // Randomize Julia Set
    {
        zoom = 0.8f + (rand() % 10) / 10.0f; // 0.8 to 1.8
        // Randomize the constant 'C' to change the island shapes entirely
        cRe = -0.8f + (rand() % 100) / 100.0f; // -0.8 to 0.2
        cIm = 0.4f + (rand() % 80) / 100.0f;   // 0.2 to 0.7
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

