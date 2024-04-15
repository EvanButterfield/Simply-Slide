static void
GetIntFromIni(LPCWSTR Section, LPCWSTR Key, LPCWSTR FileName, int *Value)
{
    int Data = GetPrivateProfileIntW(Section, Key, -1, FileName);
    if(Data >= 0)
    {
        *Value = Data;
    }
}

static void
WriteIntToIni(LPCWSTR Section, LPCWSTR Key, LPCWSTR FileName, int Value)
{
    WCHAR Text[1024];
    wsprintfW(Text, L"%u", Value);
    WritePrivateProfileStringW(Section, Key, Text, FileName);
}

static void
LoadAppFromConfig(ss_state *State, LPCWSTR FileName)
{
    WCHAR FullFileName[MAX_PATH];
    wcscpy(FullFileName, State->EXEPath);
    wcscat(FullFileName, L"\\");
    wcscat(FullFileName, FileName);
    WCHAR LastProjectPath[MAX_PATH];
    GetPrivateProfileStringW(L"ss", L"last_project", 0,
                             LastProjectPath, MAX_PATH, FullFileName);
    char LastProjectPathCStr[MAX_PATH];
    wcstombs(LastProjectPathCStr, LastProjectPath, MAX_PATH);
    if(PathFileExistsA(LastProjectPathCStr))
    {
        wcscpy(State->SaveFileName, LastProjectPath);
    }
    
    GetPrivateProfileStringW(L"ss", L"placeholder_image", 0,
                             State->PlaceholderImageName, MAX_PATH, FullFileName);
    WCHAR FullImageName[MAX_PATH];
    wcscpy(FullImageName, State->EXEPath);
    wcscat(FullImageName, L"\\");
    wcscat(FullImageName, State->PlaceholderImageName);
    wcscpy(State->PlaceholderImageName, FullImageName);
    char PlaceholderImageNameCStr[MAX_PATH];
    wcstombs(PlaceholderImageNameCStr, State->PlaceholderImageName, MAX_PATH);
    int ImageWidth;
    int ImageHeight;
    SDL_assert(LoadTexture(PlaceholderImageNameCStr, &State->PlaceholderImageTexture, &ImageWidth, &ImageHeight));
    State->PlaceholderImageSize = ImVec2((float)ImageWidth, (float)ImageHeight);
    
    wcscpy(State->ConfigFileName, FullFileName);
}

static void
WriteConfig(ss_state *State, LPCWSTR FullFileName)
{
    WritePrivateProfileStringW(L"ss", L"last_project",
                               State->SaveFileName, FullFileName);
}

static void
GetProjectFromIni(ss_state *State)
{
    LPCWSTR FileName = State->SaveFileName;
    
    WCHAR ProjectTitle[SS_TEXT_LENGTH];
    GetPrivateProfileStringW(L"ss", L"title", 0,
                             ProjectTitle, SS_TEXT_LENGTH, FileName);
    wcstombs(State->ProjectName, ProjectTitle, SS_TEXT_LENGTH);
    
    GetIntFromIni(L"ss", L"count", FileName, &State->SlideCount);
    GetIntFromIni(L"ss", L"index", FileName, &State->SlideIndex);
    ALLOC_STRUCT(State->SlideCount, &State->Slides, ss_slide);
    
    for(int SlideIndex = 0;
        SlideIndex < State->SlideCount;
        ++SlideIndex)
    {
        ss_slide *Slide = &State->Slides[SlideIndex];
        WCHAR SlideNameBuffer[1024];
        _snwprintf(SlideNameBuffer, 1024, L"slide%d", SlideIndex);
        
        int BackgroundR;
        int BackgroundG;
        int BackgroundB;
        GetIntFromIni(SlideNameBuffer, L"backgroundr", FileName, &BackgroundR);
        GetIntFromIni(SlideNameBuffer, L"backgroundg", FileName, &BackgroundG);
        GetIntFromIni(SlideNameBuffer, L"backgroundb", FileName, &BackgroundB);
        
        GetIntFromIni(SlideNameBuffer, L"text_count", FileName, &Slide->TextCount);
        GetIntFromIni(SlideNameBuffer, L"image_count", FileName, &Slide->ImageCount);
        
        Slide->BGColor = IM_COL32(BackgroundR,
                                  BackgroundG,
                                  BackgroundB,
                                  255);
        
        for(int TextIndex = 0;
            TextIndex < Slide->TextCount;
            ++TextIndex)
        {
            WCHAR TextNameBuffer[1024];
            _snwprintf(TextNameBuffer, 1024, L"text%d", TextIndex);
            
            WCHAR XBuffer[1024];
            int XPos = 0;
            _snwprintf(XBuffer, 1024, L"text%dx", TextIndex);
            WCHAR YBuffer[1024];
            int YPos = 0;
            _snwprintf(YBuffer, 1024, L"text%dy", TextIndex);
            
            WCHAR RBuffer[1024];
            int R;
            _snwprintf(RBuffer, 1024, L"text%dr", TextIndex);
            WCHAR GBuffer[1024];
            int G;
            _snwprintf(GBuffer, 1024, L"text%dg", TextIndex);
            WCHAR BBuffer[1024];
            int B;
            _snwprintf(BBuffer, 1024, L"text%db", TextIndex);
            
            WCHAR SizeBuffer[1024];
            _snwprintf(SizeBuffer, 1024, L"text%dsize", TextIndex);
            
            GetPrivateProfileStringW(SlideNameBuffer, TextNameBuffer, 0,
                                     Slide->Texts[TextIndex].Text, SS_TEXT_LENGTH, FileName);
            GetIntFromIni(SlideNameBuffer, XBuffer, FileName, &XPos);
            GetIntFromIni(SlideNameBuffer, YBuffer, FileName, &YPos);
            Slide->Texts[TextIndex].Position.x = (float)XPos;
            Slide->Texts[TextIndex].Position.y = (float)YPos;
            
            GetIntFromIni(SlideNameBuffer, RBuffer, FileName, &R);
            GetIntFromIni(SlideNameBuffer, GBuffer, FileName, &G);
            GetIntFromIni(SlideNameBuffer, BBuffer, FileName, &B);
            Slide->Texts[TextIndex].Color = IM_COL32(R, G, B, 255);
            
            int Size;
            GetIntFromIni(SlideNameBuffer, SizeBuffer, FileName, &Size);
            Slide->Texts[TextIndex].Size = (float)Size;
        }
        
        for(int ImageIndex = 0;
            ImageIndex < Slide->ImageCount;
            ++ImageIndex)
        {
            WCHAR ImageNameBuffer[1024];
            _snwprintf(ImageNameBuffer, 1024, L"image%d", ImageIndex);
            
            WCHAR PathNameBuffer[1024];
            _snwprintf(PathNameBuffer, 1024, L"%s_path", ImageNameBuffer);
            
            WCHAR XPosBuffer[1024];
            _snwprintf(XPosBuffer, 1024, L"%s_pos_x", ImageNameBuffer);
            WCHAR YPosBuffer[1024];
            _snwprintf(YPosBuffer, 1024, L"%s_pos_y", ImageNameBuffer);
            
            WCHAR WidthBuffer[1024];
            _snwprintf(WidthBuffer, 1024, L"%s_width", ImageNameBuffer);
            
            WCHAR HeightBuffer[1024];
            _snwprintf(HeightBuffer, 1024, L"%s_height", ImageNameBuffer);
            
            GetPrivateProfileStringW(SlideNameBuffer, PathNameBuffer, 0,
                                     Slide->Images[ImageIndex].FilePath, MAX_PATH, FileName);
            int ImageWidth = 0;
            int ImageHeight = 0;
            char FilePath[MAX_PATH];
            wcstombs(FilePath, Slide->Images[ImageIndex].FilePath, MAX_PATH);
            LoadTexture(FilePath,
                        &Slide->Images[ImageIndex].Tex,
                        &ImageWidth, &ImageHeight);
            Slide->Images[ImageIndex].Size = ImVec2((float)ImageWidth, (float)ImageHeight);
            
            int PosX = 0;
            int PosY = 0;
            GetIntFromIni(SlideNameBuffer, XPosBuffer, FileName, &PosX);
            GetIntFromIni(SlideNameBuffer, YPosBuffer, FileName, &PosY);
            Slide->Images[ImageIndex].Position = ImVec2((float)PosX, (float)PosY);
            
            int Width = 0;
            int Height = 0;
            GetIntFromIni(SlideNameBuffer, WidthBuffer, FileName, &Width);
            GetIntFromIni(SlideNameBuffer, HeightBuffer, FileName, &Height);
            Slide->Images[ImageIndex].Size = ImVec2((float)Width, (float)Height);
        }
    }
}

static void
WriteProjectToIni(ss_state *State, LPCWSTR FileName)
{
    WCHAR ProjectTitle[SS_TEXT_LENGTH];
    mbstowcs(ProjectTitle, State->ProjectName, SS_TEXT_LENGTH);
    WritePrivateProfileStringW(L"ss", L"title", ProjectTitle, FileName);
    
    WriteIntToIni(L"ss", L"count", FileName, State->SlideCount);
    WriteIntToIni(L"ss", L"index", FileName, State->SlideIndex);
    
    for(int SlideIndex = 0;
        SlideIndex < State->SlideCount;
        ++SlideIndex)
    {
        ss_slide *Slide = &State->Slides[SlideIndex];
        WCHAR SlideNameBuffer[1024];
        _snwprintf(SlideNameBuffer, 1024, L"slide%d", SlideIndex);
        
        unsigned short BGR = (Slide->BGColor >> IM_COL32_R_SHIFT) & 0xFF;
        unsigned short BGG = (Slide->BGColor >> IM_COL32_G_SHIFT) & 0xFF;
        unsigned short BGB = (Slide->BGColor >> IM_COL32_B_SHIFT) & 0xFF;
        
        WriteIntToIni(SlideNameBuffer, L"backgroundr", FileName, BGR);
        WriteIntToIni(SlideNameBuffer, L"backgroundg", FileName, BGG);
        WriteIntToIni(SlideNameBuffer, L"backgroundb", FileName, BGB);
        
        WriteIntToIni(SlideNameBuffer, L"text_count", FileName, Slide->TextCount);
        WriteIntToIni(SlideNameBuffer, L"image_count", FileName, Slide->ImageCount);
        
        for(int TextIndex = 0;
            TextIndex < Slide->TextCount;
            ++TextIndex)
        {
            WCHAR TextNameBuffer[1024];
            _snwprintf(TextNameBuffer, 1024, L"text%d", TextIndex);
            WCHAR XBuffer[1024];
            int XPos = (int)Slide->Texts[TextIndex].Position.x;
            _snwprintf(XBuffer, 1024, L"text%dx", TextIndex);
            WCHAR YBuffer[1024];
            int YPos = (int)Slide->Texts[TextIndex].Position.y;
            _snwprintf(YBuffer, 1024, L"text%dy", TextIndex);
            
            WCHAR RBuffer[1024];
            unsigned short R = (Slide->Texts[TextIndex].Color >>
                                IM_COL32_R_SHIFT) & 0xFF;
            _snwprintf(RBuffer, 1024, L"text%dr", TextIndex);
            WCHAR GBuffer[1024];
            unsigned short G = (Slide->Texts[TextIndex].Color >>
                                IM_COL32_G_SHIFT) & 0xFF;
            _snwprintf(GBuffer, 1024, L"text%dg", TextIndex);
            WCHAR BBuffer[1024];
            unsigned short B = (Slide->Texts[TextIndex].Color >>
                                IM_COL32_B_SHIFT) & 0xFF;
            _snwprintf(BBuffer, 1024, L"text%db", TextIndex);
            
            WCHAR SizeBuffer[1024];
            _snwprintf(SizeBuffer, 1024, L"text%dsize", TextIndex);
            
            WritePrivateProfileStringW(SlideNameBuffer, TextNameBuffer,
                                       Slide->Texts[TextIndex].Text, FileName);
            WriteIntToIni(SlideNameBuffer, XBuffer, FileName, XPos);
            WriteIntToIni(SlideNameBuffer, YBuffer, FileName, YPos);
            
            WriteIntToIni(SlideNameBuffer, RBuffer, FileName, R);
            WriteIntToIni(SlideNameBuffer, GBuffer, FileName, G);
            WriteIntToIni(SlideNameBuffer, BBuffer, FileName, B);
            
            WriteIntToIni(SlideNameBuffer, SizeBuffer, FileName,
                          (int)roundf(Slide->Texts[TextIndex].Size));
        }
        
        for(int ImageIndex = 0;
            ImageIndex < Slide->ImageCount;
            ++ImageIndex)
        {
            WCHAR ImageNameBuffer[1024];
            _snwprintf(ImageNameBuffer, 1024, L"image%d", ImageIndex);
            
            WCHAR PathNameBuffer[1024];
            _snwprintf(PathNameBuffer, 1024, L"%s_path", ImageNameBuffer);
            
            WCHAR XPosBuffer[1024];
            _snwprintf(XPosBuffer, 1024, L"%s_pos_x", ImageNameBuffer);
            WCHAR YPosBuffer[1024];
            _snwprintf(YPosBuffer, 1024, L"%s_pos_y", ImageNameBuffer);
            
            WCHAR WidthBuffer[1024];
            _snwprintf(WidthBuffer, 1024, L"%s_width", ImageNameBuffer);
            
            WCHAR HeightBuffer[1024];
            _snwprintf(HeightBuffer, 1024, L"%s_height", ImageNameBuffer);
            
            WritePrivateProfileStringW(SlideNameBuffer, PathNameBuffer,
                                       Slide->Images[ImageIndex].FilePath, FileName);
            
            int PosX = (int)Slide->Images[ImageIndex].Position.x;
            int PosY = (int)Slide->Images[ImageIndex].Position.y;
            WriteIntToIni(SlideNameBuffer, XPosBuffer, FileName, PosX);
            WriteIntToIni(SlideNameBuffer, YPosBuffer, FileName, PosY);
            
            int Width = (int)Slide->Images[ImageIndex].Size.x;
            int Height = (int)Slide->Images[ImageIndex].Size.y;
            WriteIntToIni(SlideNameBuffer, WidthBuffer, FileName, Width);
            WriteIntToIni(SlideNameBuffer, HeightBuffer, FileName, Height);
        }
    }
}