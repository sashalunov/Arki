#include "stdafx.h"
#include "HL1WADStructures.h"
#include "WADViewer.h"
#include "imgui.h"

CWADViewer::CWADViewer(TextureManager* pTexMgr)
	: m_pTexMgr(pTexMgr)
{
	m_isOpen = FALSE;
}

CWADViewer::~CWADViewer() 
{
}

void CWADViewer::OpenWAD(const std::wstring& path)
{
    m_isOpen = FALSE;

	m_currentWadPath = path;
	ScanWadContent();
	m_isOpen = TRUE;
}

void CWADViewer::ScanWadContent()
{
    m_entries.clear();

    FILE* f = _wfopen(m_currentWadPath.c_str(), L"rb");
    if (!f) { fclose(f); return; }

    wadheader_t h;
    fread(&h, sizeof(wadheader_t), 1, f);
    if (h.ident != WAD3_ID) { fclose(f); return; }

    // Read Directory
    std::vector<wadlump_t> lumps(h.numlumps);
    fseek(f, h.diroffset, SEEK_SET);
    fread(lumps.data(), sizeof(wadlump_t), h.numlumps, f);

    // Parse Directory
    for (const auto& lump : lumps)
    {
        if (lump.type != 0x43) continue; // Skip non-textures (0x43 = MipTex)

        TextureEntry entry;
        std::string s = lump.name;
        entry.name = std::wstring(s.begin(),s.end());
        entry.pTexture = NULL; // Not loaded yet

        // Jump to Lump to read width/height from bspmiptex_t
        fseek(f, lump.filepos, SEEK_SET);
        bspmiptex_t mt;
        fread(&mt, sizeof(bspmiptex_t), 1, f);

        entry.width = mt.width;
        entry.height = mt.height;

        m_entries.push_back(entry);
    }
    fclose(f);
}

void CWADViewer::Draw(BOOL* pOpen)
{
   if(!m_isOpen)
		return;

    ImGui::Begin("WAD Viewer");
    ImGui::Text("File: %s", WToUTF8(m_currentWadPath));
    ImGui::Text("Textures: %d", (int)m_entries.size());
    ImGui::Separator();
    // Optional: Thumbnail Size Slider
    static float thumbSize = 80.0f;
    ImGui::SliderFloat("Zoom", &thumbSize, 32.0f, 256.0f, "%.0f px");
    ImGui::Separator();


    // ---------------------------------------------------------
    // SCROLLABLE GRID REGION
    // ---------------------------------------------------------
    // BeginChild creates a scrollable region. 
    // Size (0,0) means "fill remaining space".
    if (ImGui::BeginChild("WadGrid", ImVec2(0, 0), true))
    {
        if (!m_entries.empty())
        {
            // 1. Calculate Grid Dimensions
            float padding = 8.0f;
            float cellSize = thumbSize + padding;

            // Available width inside the child window
            float windowVisibleX = ImGui::GetContentRegionAvail().x;

            // How many columns fit? (Ensure at least 1)
            int colCount = (int)(windowVisibleX / cellSize);
            if (colCount < 1) colCount = 1;

            // How many rows total?
            int totalItems = (int)m_entries.size();
            int rowCount = (int)ceil((float)totalItems / colCount);

            // 2. Setup Clipper
            // We tell clipper how many ROWS we have, and the height of each row.
            ImGuiListClipper clipper;
            clipper.Begin(rowCount, cellSize);


            while (clipper.Step())
            {
                // Iterate over visible rows
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    // For each row, iterate columns
                    for (int col = 0; col < colCount; col++)
                    {
                        // Calculate actual index in the vector
                        int itemIdx = row * colCount + col;

                        // Stop if we ran out of items (last row might be incomplete)
                        if (itemIdx >= totalItems) break;

                        TextureEntry& item = m_entries[itemIdx];

                        // --- DRAW ITEM ---
                        ImGui::PushID(itemIdx);
                        ImGui::BeginGroup(); // Group Image + Text together

                        // A. Lazy Load Texture
                        if (item.pTexture == NULL)
                        {
                            m_pTexMgr->LoadWAD(m_currentWadPath);
                            item.pTexture = m_pTexMgr->GetTexture(item.name);
                        }

                        // B. Draw Image
                        if (item.pTexture)
                        {
                            // Draw Image centered in cell
                            float availWidth = thumbSize;
                            float xPos = ImGui::GetCursorPosX();

                            // Image Button allows clicking (selection) logic later if needed
                            // Note: We use -1 for UVs to flip if needed, but usually 0,0,1,1 is standard
                            if (ImGui::ImageButton("##tex", (void*)item.pTexture, ImVec2(thumbSize, thumbSize)))
                            {
                                // Click logic here (e.g. copy name to clipboard)
                                ImGui::SetClipboardText(WToUTF8(item.name));
                            }

                            // Tooltip on Hover
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                float aspect = (float)item.width / (float)item.height;
                                // Show large preview (max 256 wide)
                                float previewW = 256.0f;
                                float previewH = 256.0f / aspect;
                                ImGui::Text("%s (%dx%d)", WToUTF8(item.name), item.width, item.height);
                                ImGui::Image((void*)item.pTexture, ImVec2(previewW, previewH));
                                ImGui::EndTooltip();
                            }
                        }
                        else
                        {
                            // Placeholder if loading fails or pending
                            ImGui::Button("?", ImVec2(thumbSize, thumbSize));
                        }

                        // C. Draw Text (Name)
                        // Limit text width to thumbnail size
                        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + thumbSize);
                        ImGui::TextWrapped("%s", WToUTF8(item.name));
                        ImGui::PopTextWrapPos();

                        ImGui::EndGroup();
                        ImGui::PopID();

                        // D. Grid Layout Logic
                        // Only add SameLine if it's NOT the last column
                        if (col < colCount - 1)
                        {
                            ImGui::SameLine();
                        }
                    }
                }
            }

        }
    }
    ImGui::EndChild();
   

    ImGui::End();
}